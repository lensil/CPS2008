class Commands:
    def __init__(self):
        self.draw_commands = []
        self.command_id = 0
        self.selected_command_id = None
    
    def apply_draw_command(self, canvas, command):

        
        parts = command.strip().split()
        if len(parts) < 6:
            print(f"Invalid command format or missing arguments: '{command}'")
            return

        # Extract the shape type from the command
        shape = parts[1]

        print("Shape: ", shape) # Debugging
        try:
            if shape == "text":
                x1, y1 = map(int, parts[2:4])
                color = parts[-1]  # The color is the last part
                text = ' '.join(parts[4:-1])  # The text is everything between the coordinates and the color
                text = text.strip("'")  # Remove the quotes around the text
                canvas.create_text(x1, y1, text=text, fill=color)
                return

            # Convert coordinates and extract color
            x1, y1, x2, y2 = map(int, parts[2:6])
            color = parts[6] if len(parts) > 6 else 'black'  # Default color if not specified


            # Draw the shape based on type
            if shape == "line":
                canvas.create_line(x1, y1, x2, y2, fill=color)
            elif shape == "rectangle":
                canvas.create_rectangle(x1, y1, x2, y2, outline=color)
            elif shape == "circle":
                canvas.create_oval(x1, y1, x2, y2, outline=color)
            elif shape == "text":
                text = parts[7] if len(parts) > 7 else "Sample Text"
                canvas.create_text(x1, y1, text=text, fill=color)

            else:
                print(f"Unsupported shape type: '{shape}' in command: '{command}'")
        except ValueError as e:
            print(f"Error parsing command: '{command}' - ValueError: {e}")
        except IndexError as e:
            print(f"Index error with command: '{command}' - IndexError: {e}")
        except Exception as e:
            print(f"Unexpected error processing command: '{command}' - Exception: {e}")

    def redraw(self, canvas, filter_user=None):
        canvas.delete("all")
        for _, command in self.draw_commands:
            if filter_user:
                # Logic to filter by user if implemented
                pass
            self.apply_draw_command(canvas, command, redraw=True)
    
    def list_commands(self, filter_tool=None, filter_user=None):
        filtered_commands = []
        for cmd_id, command in self.draw_commands:
            if filter_tool and filter_tool not in command:
                continue
            if filter_user:
                # Logic to filter by user if implemented
                pass
            filtered_commands.append((cmd_id, command))
        return filtered_commands
    
    def delete_command(self, canvas, cmd_id):
        self.draw_commands = [cmd for cmd in self.draw_commands if cmd[0] != cmd_id]
        self.redraw(canvas)
    
    def undo_last(self, canvas):
        if self.draw_commands:
            self.draw_commands.pop()
            self.redraw(canvas)