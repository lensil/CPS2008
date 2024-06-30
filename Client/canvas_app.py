import tkinter as tk
import socket
import threading
import sys
import select
from commands import Commands

class CanvasApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Shared Canvas")
        self.user_commands = set()
        self.shape_id_counter = 0
        self.selected_command_id = None

        # Create canvas
        self.canvas = tk.Canvas(root, width=800, height=600, bg="white")
        self.canvas.pack()

        # Setup server connection
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect(('127.0.0.1', 6001))
        self.client_socket.settimeout(0.1)  # Set a short timeout for non-blocking operations

        # Initialize Commands
        self.commands = Commands()

        # Start receiving thread
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread.start()

        # Initialize current tool and color
        self.current_tool = None
        self.current_color = None

        # Start checking for terminal input
        self.root.after(100, self.check_terminal_input)

    def check_terminal_input(self):
        if select.select([sys.stdin], [], [], 0.0)[0]:
            command = sys.stdin.readline().strip()
            self.execute_command(command)
        self.root.after(100, self.check_terminal_input)

    def execute_command(self, command):
        parts = command.split()
        if not parts:
            return

        cmd = parts[0]
        if cmd == "tool":
            self.current_tool = parts[1] if len(parts) > 1 else None
            print(f"Current tool set to: {self.current_tool}")
        elif cmd == "colour":
            if len(parts) != 4:
                print("Invalid color command. Usage: colour <R> <G> <B>")
                return
            self.current_color = ' '.join(parts[1:]) if len(parts) > 1 else None
            print(f"Current color set to: {self.current_color}")
        elif cmd == "draw":
            if self.current_tool == "text":
                if len(parts) < 3:
                    print("Invalid draw command for text. Usage: draw <x> <y> <text>")
                    return
                x1, y1 = int(parts[1]), int(parts[2])
                text = ' '.join(parts[3:])
                color = self.rgb_to_hex(self.current_color) if self.current_color else "black"
                shape_id = self.canvas.create_text(x1, y1, text=text, fill=color)
                command = f"draw text {shape_id} {x1} {y1} '{text}' {self.current_color}\n"
                try:
                    self.client_socket.sendall(command.encode())
                    print(f"Sent command: {command}")
                except socket.error as e:
                    print(f"Socket error: {e}")
            else:
                if len(parts) < 5:
                    print("Invalid draw command. Usage: draw <x1> <y1> <x2> <y2>")
                    return
                if not self.current_tool:
                    print("Please select a tool first using the 'tool' command.")
                    return
                x1, y1, x2, y2 = map(int, parts[1:5])
                color = self.current_color or "black"
                self.draw_shape(self.current_tool, x1, y1, x2, y2, color)
        elif cmd == "help":
            self.show_help()
        elif cmd == "list":
            filter_tool, filter_user = parts[1], parts[2]
            send_command = f"list {filter_tool} {filter_user}\n"
            try:
                self.client_socket.sendall(send_command.encode())
                print(f"Sent command: {send_command}")
            except socket.error as e:
                print(f"Socket error: {e}")
        elif cmd == "modify":
            self.modify_command(parts[1:])
        elif cmd == "delete":
            if len(parts) > 1:
                self.commands.delete_command(self.canvas, int(parts[1]))
            self.user_commands.discard(int(parts[1])) 
            delete_command = f"delete {parts[1]}\n"
            try:
                self.client_socket.sendall(delete_command.encode())
                print(f"Sent command: {delete_command}")
            except socket.error as e:
                print(f"Socket error: {e}")
        elif cmd == "undo":
            self.commands.undo_last(self.canvas)
        elif cmd == "clear":
            if parts[1] == "all":
                self.canvas.delete("all")
                self.user_commands.clear()
                print("All shapes cleared from the canvas")
                try:
                    self.client_socket.sendall("clear all\n".encode())
                except socket.error as e:
                    print(f"Socket error: {e}")
            elif parts[1] == "mine":
                for shape_id in list(self.user_commands):
                    self.canvas.delete(shape_id)
                    self.commands.delete_command(self.canvas, shape_id)
                print("User's shapes cleared from the canvas")
                try:
                    command = "clear mine "
                    for shape_id in self.user_commands:
                        command += f"{shape_id} "
                    command += "\n"
                    print(f"Sending command: {command}")
                    self.client_socket.sendall(command.encode())
                except socket.error as e:
                    print(f"Socket error: {e}")
                self.user_commands.clear()
        elif cmd == "show":
            self.show_commands(parts[1] if len(parts) > 1 else "all")
        elif cmd == "exit":
            self.client_socket.close()
            self.root.quit()
        elif cmd == "select":
            if len(parts) > 1:
                self.commands.selected_command_id = int(parts[1])
                print(f"Selected command ID: {self.commands.selected_command_id}")
        else:
            print(f"Unknown command: {cmd}")

    def modify_command(self, args):
        if self.commands.selected_command_id is None:
            return "No shape selected. Use 'select' command first."

        print(f"Selected command ID: {self.commands.selected_command_id}")

        try:

            # Construct the modification command as a single string
            modify_cmd = f"modify {self.commands.selected_command_id} {' '.join(args)}"
            print(f"Sending command: {modify_cmd}")
            self.client_socket.sendall(modify_cmd.encode())

            # Apply the modification locally
            result = self.commands.modify_command(self.canvas, args)
            return result
        except socket.error as e:
            print(f"Socket error: {e}")
            return f"Error: {e}"

    def rgb_to_hex(self, rgb):
        # Convert RGB string to hex
        rgb = tuple(map(int, rgb.split()))
        return '#{:02x}{:02x}{:02x}'.format(rgb[0], rgb[1], rgb[2])

    def draw_shape(self, shape, x1, y1, x2, y2, color):
        rgb_colour = color
        # Convert color to hex format
        try:
            color = self.rgb_to_hex(color)
        except:
            print(f"Invalid color format: {color}")
            return

        if shape == "line":
            shape_id = self.canvas.create_line(x1, y1, x2, y2, fill=color)
        elif shape == "rectangle":
            shape_id = self.canvas.create_rectangle(x1, y1, x2, y2, outline=color)
        elif shape == "circle":
            shape_id = self.canvas.create_oval(x1, y1, x2, y2, outline=color)
        elif shape == "text":
            shape_id = text = input("Enter text: ")  # This will prompt in the terminal
            self.canvas.create_text(x1, y1, text=text, fill=color)
            command = f"draw text {shape_id} {x1} {y1} '{text}' {color}\n"
        else:
            print(f"Unsupported shape: {shape}")
            return
        
        command = f"draw {shape} {shape_id} {x1} {y1} {x2} {y2} {rgb_colour}\n"
        
        # Add the shape_id to user_commands
        self.user_commands.add(shape_id)
        
        try:
            self.client_socket.sendall(command.encode())
            print(f"Sent command: {command}")
        except socket.error as e:
            print(f"Socket error: {e}")

    def receive_data(self):
        while True:
            try:
                message = self.client_socket.recv(1024).decode()
                commands = message.split('END\n')  # Split by delimiter
                for command in commands:
                    if command:
                        #print(f"Received command: {command}")
                        self.root.after(0, self.commands.apply_draw_command, self.canvas, command)
            except socket.timeout:
                continue
            except socket.error as e:
                print(f"Socket error: {e}")
                break
            except Exception as e:
                print(f"Unexpected error: {e}")
                break

        self.client_socket.close()
        print("Socket closed, attempting to reconnect...")
        self.reinitialize_connection()

    def show_commands(self, filter_type):
        if filter_type == "all":
            for shape_id in self.commands.shapes:
                self.canvas.itemconfigure(shape_id, state='normal')
        elif filter_type == "mine":
            for shape_id in self.commands.shapes:
                if shape_id in self.user_commands:
                    self.canvas.itemconfigure(shape_id, state='normal')
                else:
                    self.canvas.itemconfigure(shape_id, state='hidden')

    def show_help(self):
        help_text = """
        Available Commands:
        - help: Lists all available commands and their usage.
        - tool {line | rectangle | circle | text}: Selects a tool for drawing.
        - colour {RGB}: Sets the drawing color using RGB values (e.g., "255 0 0" for red).
        - draw <x1> <y1> <x2> <y2>: Executes the drawing of the selected shape on the canvas.
        - list {all | line | rectangle | circle | text} {all | mine}: Displays issued draw commands in the console.
        - select {none | ID}: Selects an existing draw command to be modified.
        - delete {ID}: Deletes the draw command with the specified ID.
        - undo: Reverts the user's last action.
        - clear {all | mine}: Clears the canvas.
        - show {all | mine}: Controls what is displayed on the client's canvas.
        - exit: Disconnects from the server and exits the application.
        """
        print(help_text)

    def reinitialize_connection(self):
        try:
            self.client_socket.close()
            print("Old socket closed. Reinitializing connection...")
        except Exception as e:
            print(f"Failed to close old socket: {e}")

        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(('127.0.0.1', 6001))
            self.client_socket.settimeout(0.1)
            print("Reconnected to the server.")
        except socket.error as e:
            print(f"Failed to reconnect: {e}")