class Commands:
    def __init__(self):
        self.shapes = {}
        self.draw_commands = []
        self.command_id = 0
        self.selected_command_id = None
        self.user_commands = set()  

    def rgb_to_hex(self, r, g, b):
        return '#{:02x}{:02x}{:02x}'.format(r, g, b)
    
    def apply_draw_command(self, canvas, command, redraw=False):
        parts = command.strip().split()
        if parts[0] == "list":
            list_commands = command.split("list")[1:]  # Split by "list" and remove the first empty part
            for list_cmd in list_commands:
                cmd_parts = list_cmd.strip().split('=>')
                if len(cmd_parts) == 2:
                    list_id = cmd_parts[0].strip()
                    list_item = cmd_parts[1].strip()
                    print(f"[{list_id}] => {list_item}")
            return
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
                x1, y1 = map(int, parts[3:5])
                text = parts[5]
                if len(parts) >= 9:  # Ensure we have enough parts for RGB values
                    r, g, b = map(int, parts[6:9])
                    color = self.rgb_to_hex(r, g, b)
                else:
                    color = '#000000'  # Default to black if RGB values are not provided
                shape_id = canvas.create_text(x1, y1, text=text, fill=color)
            else:
                x1, y1, x2, y2 = map(int, parts[3:7])
                if len(parts) >= 10:  # Ensure we have enough parts for RGB values
                    r, g, b = map(int, parts[7:10])
                    color = self.rgb_to_hex(r, g, b)
                else:
                    color = '#000000'  # Default to black if RGB values are not provided

            print(f"Color: {color}")
            if shape == "line":
                shape_id = canvas.create_line(x1, y1, x2, y2, fill=color)
            elif shape == "rectangle":
                shape_id = canvas.create_rectangle(x1, y1, x2, y2, outline=color)
            elif shape == "circle":
                shape_id = canvas.create_oval(x1, y1, x2, y2, outline=color)
            else:
                print(f"Unsupported shape type: '{shape}'")   
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
        print(f"Filtering commands by tool: {filter_tool} and user: {filter_user}")
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
        #self.draw_commands = [cmd for cmd in self.draw_commands if cmd[0] != shape_id]
        #if shape_id in self.shapes:
            #del self.shapes[shape_id]  # Remove the shape from the dictionary
        #self.redraw(canvas)
    
    def undo_last(self, canvas):
        if self.draw_commands:
            self.draw_commands.pop()
            #self.redraw(canvas)
