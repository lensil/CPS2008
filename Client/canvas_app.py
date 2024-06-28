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
        elif cmd == "select":
            self.commands.selected_command_id = int(parts[1]) if len(parts) > 1 and parts[1] != "none" else None
        elif cmd == "delete":
            if len(parts) > 1:
                self.commands.delete_command(self.canvas, int(parts[1]))

            delete_command = f"delete {parts[1]}\n"
            try:
                self.client_socket.sendall(delete_command.encode())
                print(f"Sent command: {delete_command}")
            except socket.error as e:
                print(f"Socket error: {e}")
        elif cmd == "undo":
            self.commands.undo_last(self.canvas)
        elif cmd == "clear":
            user_filter = parts[1] if len(parts) > 1 else None
            self.commands.draw_commands = [cmd for cmd in self.commands.draw_commands if user_filter and "mine" in user_filter]
            self.commands.redraw(self.canvas)
        elif cmd == "show":
            self.show_commands(parts[1] if len(parts) > 1 else "all")
        elif cmd == "exit":
            self.client_socket.close()
            self.root.quit()
        else:
            print(f"Unknown command: {cmd}")

    def rgb_to_hex(self, rgb):
        # Convert RGB string to hex
        rgb = tuple(map(int, rgb.split()))
        return '#{:02x}{:02x}{:02x}'.format(rgb[0], rgb[1], rgb[2])

    def draw_shape(self, shape, x1, y1, x2, y2, color):
        self.shape_id_counter += 1
        shape_id = self.shape_id_counter
        command = f"draw {shape} {shape_id} {x1} {y1} {x2} {y2} {color}\n"
        print(f"context: {command}")
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