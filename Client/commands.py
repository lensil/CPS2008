class Commands:
    def __init__(self):
        self.shapes = {}
        self.draw_commands = []
        self.command_id = 0
        self.selected_command_id = None
        self.user_commands = set()  
    
    def apply_draw_command(self, canvas, command, redraw=False):
        print(f"Drawing commands array: {self.draw_commands}")
        print(f"Shapes array: {self.shapes}")
        parts = command.strip().split()
        if parts[0] == "delete":
            shape_id = int(parts[1])
            self.delete_command(canvas, shape_id)
            return
        if len(parts) < 7:
            print(f"Invalid command format or missing arguments: '{command}'")
            return

        shape = parts[1]
        try:
            if shape == "text":
                x1, y1 = map(int, parts[2:4])
                color = parts[-1]  # The color is the last part
                text = ' '.join(parts[4:-1])  # The text is everything between the coordinates and the color
                text = text.strip("'")  # Remove the quotes around the text
                shape_id = canvas.create_text(x1, y1, text=text, fill=color)
            else:
                x1, y1, x2, y2 = map(int, parts[3:7])
                color = parts[7] if len(parts) > 6 else 'black'  # Default color if not specified

                if shape == "line":
                    shape_id = canvas.create_line(x1, y1, x2, y2, fill=color)
                    print(f"Created line with ID: {shape_id}")
                    print(f"Shapes array: {self.shapes}")
                    print(f"Draw commands array: {self.draw_commands}")
                elif shape == "rectangle":
                    shape_id = canvas.create_rectangle(x1, y1, x2, y2, outline=color)
                elif shape == "circle":
                    shape_id = canvas.create_oval(x1, y1, x2, y2, outline=color)
                else:
                    print(f"Unsupported shape type: '{shape}' in command: '{command}'")
                    return

            if not redraw:
                print("Adding shape...")
                self.draw_commands.append((shape_id, command))
                self.shapes[shape_id] = command
                self.command_id += 1

            return shape_id  # Return the new shape_id

        except ValueError as e:
            print(f"Error parsing command: '{command}' - ValueError: {e}")
        except IndexError as e:
            print(f"Index error with command: '{command}' - IndexError: {e}")
        except Exception as e:
            print(f"Unexpected error processing command: '{command}' - Exception: {e}")

    def add_command(self, shape_id, command):
        self.shapes[shape_id] = command
        self.draw_commands.append((shape_id, command))
        self.user_commands.add(shape_id)  # Add the shape_id to user_commands

    def redraw(self, canvas, filter_user=None):
        print("Shapes array:")
        print(self.shapes)
        print("Draw commands array:")
        print(self.draw_commands)
        canvas.delete("all")
        new_shapes = {}
        id_mapping = {}
        for old_shape_id, command in self.draw_commands:
            new_shape_id = self.apply_draw_command(canvas, command, redraw=True)
            new_shapes[new_shape_id] = command
            id_mapping[old_shape_id] = new_shape_id

        self.shapes = new_shapes  # Update shapes with new IDs
        self.update_draw_commands(id_mapping)

    def update_draw_commands(self, id_mapping):
        updated_draw_commands = []
        for old_shape_id, command in self.draw_commands:
            new_shape_id = id_mapping.get(old_shape_id)
            if new_shape_id:
                updated_draw_commands.append((new_shape_id, command))
        self.draw_commands = updated_draw_commands

    def list_commands(self, filter_tool=None, filter_user=None):
        filtered_commands = []
        for shape_id, command in self.draw_commands:
            if filter_tool and filter_tool not in command:
                continue
            if filter_user == "mine" and shape_id not in self.user_commands:
                continue
            filtered_commands.append((shape_id, command))
        return filtered_commands
    
    def delete_command(self, canvas, shape_id):
        print(f"Deleting shape with ID: {shape_id}")
        print(self.shapes)
        canvas.delete(shape_id)
        self.draw_commands = [cmd for cmd in self.draw_commands if cmd[0] != shape_id]
        if shape_id in self.shapes:
            del self.shapes[shape_id]  # Remove the shape from the dictionary
        #self.redraw(canvas)
    
    def undo_last(self, canvas):
        if self.draw_commands:
            self.draw_commands.pop()
            #self.redraw(canvas)
