import tkinter as tk
from tkinter import ttk
import socket
import threading
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

        # Add control panel
        self.control_frame = ttk.Frame(root)
        self.control_frame.pack()

        # Add tool selection
        self.tool_label = ttk.Label(self.control_frame, text="Tool:")
        self.tool_label.grid(row=0, column=0)
        self.tool_var = tk.StringVar()
        self.tool_combobox = ttk.Combobox(self.control_frame, textvariable=self.tool_var)
        self.tool_combobox['values'] = ("Line", "Rectangle", "Circle", "Text", "Delete")
        self.tool_combobox.grid(row=0, column=1)

        # Add color selection
        self.color_label = ttk.Label(self.control_frame, text="Color:")
        self.color_label.grid(row=1, column=0)
        self.color_var = tk.StringVar()
        self.color_combobox = ttk.Combobox(self.control_frame, textvariable=self.color_var)
        self.color_combobox['values'] = ("Red", "Green", "Blue")
        self.color_combobox.grid(row=1, column=1)

        # Add show controls
        self.show_label = ttk.Label(self.control_frame, text="Show:")
        self.show_label.grid(row=2, column=0)
        self.show_var = tk.StringVar()
        self.show_combobox = ttk.Combobox(self.control_frame, textvariable=self.show_var)
        self.show_combobox['values'] = ("All", "Mine")
        self.show_combobox.grid(row=2, column=1)
        self.show_combobox.bind("<<ComboboxSelected>>", self.on_show_selection)

        # Add command entry
        self.command_label = ttk.Label(self.control_frame, text="Command:")
        self.command_label.grid(row=3, column=0)
        self.command_entry = ttk.Entry(self.control_frame)
        self.command_entry.grid(row=3, column=1)
        self.command_entry.bind("<Return>", self.on_command_enter)

        # Bind events
        self.canvas.bind("<Button-1>", self.on_canvas_click)

        # Setup server connection
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.client_socket.connect(('127.0.0.1', 6001))
        self.client_socket.settimeout(2.0)  # Set a timeout for blocking socket operations

        # Start receiving thread
        self.commands = Commands()
        self.receive_thread = threading.Thread(target=self.receive_data, daemon=True)
        self.receive_thread.start()

    def on_canvas_click(self, event):
        tool = self.tool_var.get().lower()
        color = self.color_var.get().lower()
        command = ''  # Initialize command to an empty string      
        self.shape_id_counter += 1  # Increment the counter
        shape_id = self.shape_id_counter  # Use the counter as the shape ID

        if tool == "line":
            shape_id = self.canvas.create_line(event.x, event.y, event.x + 100, event.y + 100, fill=color)
            command = f"draw line {shape_id} {event.x} {event.y} {event.x + 100} {event.y + 100} {color}\n"
        elif tool == "rectangle":
            shape_id = self.canvas.create_rectangle(event.x, event.y, event.x + 100, event.y + 50, outline=color)
            command = f"draw rectangle {shape_id} {event.x} {event.y} {event.x + 100} {event.y + 50} {color}\n"
        elif tool == "circle":
            shape_id = self.canvas.create_oval(event.x, event.y, event.x + 50, event.y + 50, outline=color)
            command = f"draw circle {shape_id} {event.x} {event.y} {event.x + 50} {event.y + 50} {color}\n"
        elif tool == "text":
            shape_id = self.canvas.create_text(event.x, event.y, text="Sample Text", fill=color)
            command = f"draw text {shape_id} {event.x} {event.y} 'Sample Text' {color}\n"
        elif tool == "delete":
            shape_id = self.canvas.find_closest(event.x, event.y)[0]
            command = f"delete {shape_id}\n"
            self.commands.delete_command(self.canvas, shape_id)
        else:
            print(f"Unsupported tool type: {tool}")
            return  # Early return to avoid sending an empty command

        self.commands.shapes[shape_id] = command
        self.commands.add_command(shape_id, command)
        print(f"Added shape with ID: {shape_id}")
        print(self.commands.shapes)

        # Send command to server
        if command:
            try:
                self.client_socket.sendall(command.encode())
                self.user_commands.add(shape_id)
                print(f"Sent command: {command}")
            except socket.error as e:
                print(f"Socket error: {e}")
        else:
            print("No command to send")

    def receive_data(self):
        while True:
            try:
                message = self.client_socket.recv(1024).decode()
                commands = message.split('END\n')  # Split by delimiter
                for command in commands:
                    if command:
                        print(f"Received command: {command}")
                        #self.commands.apply_draw_command(self.canvas, command)
                        self.root.after(0, self.commands.apply_draw_command, self.canvas, command)
            except socket.timeout:
                continue  # Continue the loop on timeout
            except socket.error as e:
                print(f"Socket error: {e}")
                break
            except Exception as e:
                print(f"Unexpected error: {e}")
                break

        self.client_socket.close()
        print("Socket closed, attempting to reconnect...")
        self.reinitialize_connection()

    def on_show_selection(self, event):
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

    def on_command_enter(self, event):
        command = self.command_entry.get()
        self.execute_command(command)
        self.command_entry.delete(0, tk.END)

    def execute_command(self, command):
        parts = command.split()
        cmd = parts[0]
        if cmd == "help":
            self.show_help()
        elif cmd == "tool":
            self.tool_var.set(parts[1].capitalize())
        elif cmd == "colour":
            self.color_var.set(parts[1])
        elif cmd == "list":
            tool_filter = parts[1] if parts[1] != "all" else None
            user_filter = parts[2] if parts[2] != "all" else None
            print(self.commands.list_commands(tool_filter, user_filter))
        elif cmd == "select":
            self.commands.selected_command_id = int(parts[1]) if parts[1] != "none" else None
        elif cmd == "delete":
            self.commands.delete_command(self.canvas, int(parts[1]))
        elif cmd == "undo":
            self.commands.undo_last(self.canvas)
        elif cmd == "clear":
            user_filter = parts[1] if len(parts) > 1 else None
            self.commands.draw_commands = [cmd for cmd in self.commands.draw_commands if user_filter and "mine" in user_filter]
            self.commands.redraw(self.canvas)
        elif cmd == "show":
            self.on_show_selection(event=None)
        elif cmd == "exit":
            self.client_socket.close()
            self.root.quit()

    def show_help(self):
        help_text = """
        Available Commands:
        - help: Lists all available commands and their usage.
        - tool {line | rectangle | circle | text | delete}: Selects a tool for drawing or deleting.
        - colour {RGB}: Sets the drawing color using RGB values.
        - draw {parameters}: Executes the drawing of the selected shape on the canvas.
        - list {all | line | rectangle | circle | text} {all | mine}: Displays issued draw commands in the console.
        - select {none | ID}: Selects an existing draw command to be modified.
        - delete {ID}: Deletes the draw command with the specified ID.
        - undo: Reverts the user’s last action.
        - clear {all | mine}: Clears the canvas.
        - show {all | mine}: Controls what is displayed on the client’s canvas.
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
            self.client_socket.settimeout(2.0)
            print("Reconnected to the server.")
        except socket.error as e:
            print(f"Failed to reconnect: {e}")
