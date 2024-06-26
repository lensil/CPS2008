import tkinter as tk
from tkinter import ttk
import socket
import threading
from commands import Commands

class CanvasApp:
    def __init__(self, root):
        self.root = root
        self.user_commands = set()
        self.shape_id_counter = 0
        self.commands = Commands()
        self.current_tool = None
        self.current_color = "black"
        self.selected_id = None

        self.show_var = tk.StringVar()

        # Create canvas
        self.canvas = tk.Canvas(root, width=800, height=600, bg="white")
        self.canvas.pack()

        # Setup server connection
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect(('127.0.0.1', 6001))
        self.client_socket.settimeout(2.0)  # Set a timeout for blocking socket operations

        while True:
            nickname = input("Enter your nickname: ").strip()
            if self.set_nickname(nickname):
                break


        # Start receiving thread
        self.commands = Commands()
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread.start()

    def set_nickname(self, nickname):
        command = f"NICKNAME {nickname}"
        self.client_socket.sendall(command.encode())
        response = self.client_socket.recv(1024).decode().strip()
        if response == "NICKNAME_ACCEPTED":
            print(f"Nickname '{nickname}' accepted by the server.")
            return True
        elif response == "NICKNAME_TAKEN":
            print(f"Nickname '{nickname}' is already taken. Please choose another.")
            return False
        else:
            print(f"Unexpected response from server: {response}")
            return False

    def execute_command(self, command):
        parts = command.split()
        cmd = parts[0].lower()
        args = parts[1:]

        if cmd == 'help':
            return self.show_help()
        elif cmd == 'tool':
            return self.set_tool(args)
        elif cmd == 'colour':
            return self.set_color(args)
        elif cmd == 'draw':
            return self.draw(args)
        elif cmd == 'select':
            return self.select_command(args)
        elif cmd == 'modify':
            return self.modify_command(args)
        elif cmd == 'list':
            return self.list_commands(args)
        elif cmd == 'delete':
            return self.delete_command(args)
        elif cmd == 'undo':
            return self.undo()
        elif cmd == 'clear':
            return self.clear(args)
        elif cmd == 'show':
            return self.show(args)
        else:
            return f"Unknown command: {cmd}. Type 'help' for available commands."

    def show_help(self):
        help_text = """
        Available commands:
        help - Lists all available commands and their usage.
        tool {line | rectangle | circle | text} - Selects a tool for drawing.
        colour {R G B} - Sets the drawing color using RGB values.
        draw {parameters} - Executes the drawing of the selected shape on the canvas.
        list {all | line | rectangle | circle | text} {all | mine} - Displays issued draw commands.
        select {none | ID} - Selects an existing draw command to be modified.
        delete {ID} - Deletes the draw command with the specified ID.
        undo - Reverts the user's last action.
        clear {all | mine} - Clears the canvas.
        show {all | mine} - Controls what is displayed on the client's canvas.
        exit - Disconnects from the server and exits the application.
        """
        return help_text

    def set_tool(self, args):
        if len(args) != 1 or args[0] not in ['line', 'rectangle', 'circle', 'text']:
            return "Invalid tool. Usage: tool {line | rectangle | circle | text}"
        self.current_tool = args[0]
        return f"Tool set to: {self.current_tool}"

    def set_color(self, args):
        if len(args) != 3 or not all(arg.isdigit() and 0 <= int(arg) <= 255 for arg in args):
            return "Invalid color. Usage: colour R G B (0-255 for each value)"
        self.current_color = f"#{int(args[0]):02x}{int(args[1]):02x}{int(args[2]):02x}"
        return f"Color set to: {self.current_color}"

    def draw(self, args):
        if not self.current_tool:
            return "No tool selected. Use 'tool' command first."

        r, g, b = self.current_color

        if self.current_tool == "text":
            if len(args) < 3:
                return "Invalid arguments for text tool. Usage: draw x y 'text'"
            x, y = int(args[0]), int(args[1])
            text = ' '.join(args[2:]).strip("'")
            color = f"#{r:02x}{g:02x}{b:02x}"
            shape_id = self.canvas.create_text(x, y, text=text, fill=color)
            command = f"draw text {shape_id} {x} {y} '{text}' {r} {g} {b}"
        else:
            if len(args) != 4:
                return f"Invalid arguments for {self.current_tool} tool. Usage: draw x1 y1 x2 y2"
            x1, y1, x2, y2 = map(int, args)
            color = f"#{r:02x}{g:02x}{b:02x}"
            if self.current_tool == "line":
                shape_id = self.canvas.create_line(x1, y1, x2, y2, fill=color)
            elif self.current_tool == "rectangle":
                shape_id = self.canvas.create_rectangle(x1, y1, x2, y2, outline=color)
            elif self.current_tool == "circle":
                shape_id = self.canvas.create_oval(x1, y1, x2, y2, outline=color)
            command = f"draw {self.current_tool} {shape_id} {x1} {y1} {x2} {y2} {r} {g} {b}"

        try:
            self.client_socket.sendall(command.encode())
            self.user_commands.add(shape_id)  # Add the shape_id to user_commands
            print(f"Sent command: {command}")
        except socket.error as e:
            print(f"Socket error: {e}")
        
        self.shape_id_counter += 1
        self.commands.add_command(shape_id, command)
        self.commands.shapes[shape_id] = command
        print(f"Added shape with ID: {shape_id}")
        print(self.commands.shapes)
        return f"Drawn {self.current_tool} with ID {shape_id}"

    def set_color(self, args):
        if len(args) != 3 or not all(arg.isdigit() and 0 <= int(arg) <= 255 for arg in args):
            return "Invalid color. Usage: colour R G B (0-255 for each value)"
        self.current_color = tuple(map(int, args))
        return f"Color set to: RGB{self.current_color}"

    def select_command(self, args):
        if len(args) != 1:
            return "Invalid usage. Use: select {ID}"
        try:
            command_id = int(args[0])
            return self.commands.select_command(command_id)
        except ValueError:
            return "Invalid ID. Please provide a valid integer ID."

    def modify_command(self, args):
        if self.commands.selected_command_id is None:
            return "No shape selected. Use 'select' command first."

        print(f"Selected command ID: {self.commands.selected_command_id}")

        try:
            # Construct the modification command as a single string
            modify_cmd = f"modify {self.commands.selected_command_id} {' '.join(args)}"
            self.client_socket.sendall(modify_cmd.encode())
            response = self.client_socket.recv(1024).decode()
            print(f"Server response: {response}")

            # Apply the modification locally
            result = self.commands.modify_command(self.canvas, args)
            return result
        except socket.error as e:
            print(f"Socket error: {e}")
            return f"Error: {e}"

    def list_commands(self, args):
        if len(args) != 2:
            return "Invalid usage. Use: list {all | line | rectangle | circle | text} {all | mine}"
        filter_tool = args[0] if args[0] != 'all' else None
        filter_user = args[1]
        commands = self.commands.list_commands(filter_tool, filter_user)
        return "\n".join(commands)

    def delete_command(self, args):
        if len(args) != 1:
            return "Invalid usage. Use: delete {ID}"
        try:
            command_id = int(args[0])
            self.client_socket.sendall(f"delete {command_id}".encode())
            response = self.client_socket.recv(1024).decode()
            self.commands.delete_command(self.canvas, command_id)
            return response
        except ValueError:
            return "Invalid ID. Please provide a valid integer ID."

    def undo(self):
        self.client_socket.sendall(b"undo\n")
        response = self.client_socket.recv(1024).decode()
        self.commands.undo_last(self.canvas)
        return response

    def clear(self, args):
        if len(args) != 1 or args[0] not in ['all', 'mine']:
            return "Invalid usage. Use: clear {all | mine}"
        self.client_socket.sendall(f"clear {args[0]}\n".encode())
        response = self.client_socket.recv(1024).decode()
        if args[0] == 'all':
            self.commands.draw_commands.clear()
        else:
            self.commands.draw_commands = [cmd for cmd in self.commands.draw_commands if cmd[0] not in self.commands.user_commands]
        self.commands.redraw(self.canvas)
        return response

    def show(self, args):
        if len(args) != 1 or args[0] not in ['all', 'mine']:
            return "Invalid usage. Use: show {all | mine}"
        self.show_var.set(args[0])
        self.update_display()
        return f"Display set to show: {args[0]}"

    def update_display(self):
        selection = self.show_var.get().lower()
        if selection == "all":
            self.show_all_commands()
        elif selection == "mine":
            self.show_user_commands()

    def show_all_commands(self):
        for shape_id in self.commands.shapes:
            self.canvas.itemconfigure(shape_id, state='normal')

    def show_user_commands(self):
        for shape_id in self.commands.shapes:
            if shape_id in self.user_commands:
                self.canvas.itemconfigure(shape_id, state='normal')
            else:
                self.canvas.itemconfigure(shape_id, state='hidden')

    def receive_data(self):
        while True:
            try:
                message = self.client_socket.recv(1024).decode()
                commands = message.split('END\n')
                for command in commands:
                    if command:
                        print(f"Received command: {command}")
                        shape_id = self.commands.apply_draw_command(self.canvas, command)
                        if shape_id is not None:
                            self.root.after(0, self.update_display)
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

    def reinitialize_connection(self):
        try:
            self.client_socket.close()
            print("Old socket closed. Reinitializing connection...")
        except Exception as e:
            print(f"Failed to close old socket: {e}")

        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect(('127.0.0.1', 6001))
            self.client_socket.settimeout(2.0)
            print("Reconnected to the server.")
        except socket.error as e:
            print(f"Failed to reconnect: {e}")

