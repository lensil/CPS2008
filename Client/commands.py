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
        """
        Applies a draw command to the canvas.

        Parameters:
            canvas (Canvas): The canvas object to draw on.
            command (str): The draw command to apply.
            redraw (bool, optional): Indicates whether the command is being redrawn. Defaults to False.

        Returns:
            int: The ID of the newly created shape.

        Raises:
            ValueError: If there is an error parsing the command.
            IndexError: If there is an index error with the command.
            Exception: If there is an unexpected error processing the command.
        """
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
        if parts[0] == "clear":
            if len(parts) > 1 and parts[1] == "all":
                canvas.delete("all")
                self.shapes.clear()
                self.draw_commands.clear()
                self.user_commands.clear()
            elif len(parts) > 1 and parts[1] == "mine":
                for shape_id in parts[2:]:
                    canvas.delete(shape_id)
                    if shape_id in self.shapes:
                        del self.shapes[shape_id]
                    self.draw_commands = [(id, cmd) for id, cmd in self.draw_commands if id not in self.user_commands]
                self.user_commands.clear()
            return
        if parts[0] == "modify":
            print (f"Modifying command: {command}")
            self.selected_command_id = int(parts[1])
            return self.modify_command(canvas, parts[2:])
        if len(parts) < 7:
            print(f"Invalid command format or missing arguments: '{command}'")
            return
        shape = parts[1]
        try:
            if shape == "text":
                _, _, shape_id, x1, y1, text, r, g, b = parts
                # Remove surrounding quotes from text
                text = text.strip("'\"")
                color = self.rgb_to_hex(int(r), int(g), int(b))
                shape_id = canvas.create_text(int(x1), int(y1), text=text, fill=color)
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
            elif shape == "text":
                shape_id = canvas.create_text(x1, y1, text=text, fill=color)
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
        """
        Adds a command for a specific shape.

        Parameters:
            shape_id (int): The ID of the shape.
            command (str): The command to be executed for the shape.

        Returns:
        None
        """
        self.shapes[shape_id] = command
        self.draw_commands.append((shape_id, command))
        self.user_commands.add(shape_id)  # Add the shape_id to user_commands

    def redraw(self, canvas, filter_user=None):
        """
        Redraws the canvas by applying the draw commands stored in the `draw_commands` array.

        Parameters:
            canvas (Canvas): The canvas object to redraw on.
            filter_user (str, optional): If provided, only redraws shapes belonging to the specified user.

        Returns:
            None
        """
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
        """
        Updates the draw commands with new shape IDs based on the given ID mapping.

        Parameters:
            id_mapping (dict): A dictionary mapping old shape IDs to new shape IDs.

        Returns:
            None

        """
        updated_draw_commands = []
        for old_shape_id, command in self.draw_commands:
            new_shape_id = id_mapping.get(old_shape_id)
            if new_shape_id:
                updated_draw_commands.append((new_shape_id, command))
        self.draw_commands = updated_draw_commands

    def list_commands(self, filter_tool=None, filter_user=None):
        """
        Returns a list of commands filtered by tool and user.

        Patameters:
            filter_tool (str, optional): The tool to filter commands by. Defaults to None.
            filter_user (str, optional): The user to filter commands by. Defaults to None.

        Returns:
            list: A list of filtered commands, where each command is represented as a tuple (shape_id, command).
        """
        print(f"Filtering commands by tool: {filter_tool} and user: {filter_user}")
        filtered_commands = []
        for shape_id, command in self.draw_commands:
            parts = command.split()
            if filter_tool != "all" and filter_tool not in parts:
                continue
            if filter_user == "mine" and shape_id not in self.user_commands:
                continue
            filtered_commands.append((shape_id, command))
        return filtered_commands

    
    def delete_command(self, canvas, shape_id):
        """
        Deletes a shape from the canvas.

        Parameters:
            canvas (Canvas): The canvas object where the shape is located.
            shape_id (int): The ID of the shape to be deleted.

        Returns:
            None
        """
        print(f"Deleting shape with ID: {shape_id}")
        print(self.shapes)
        canvas.delete(shape_id)
    
    def undo_last(self, canvas):
        pass

    def modify_command(self, canvas, args):
        """
        Modifies the selected command.

        Patameters:
            canvas (Canvas): The canvas object.
            args (list): The arguments for modifying the command.

        Returns:
            str: A message indicating the result of the modification.
        """
        if self.selected_command_id is None:
            return "No command selected. Use 'select' command first."

        return self.handle_modify_command(canvas, [str(self.selected_command_id)] + args)

    def handle_modify_command(self, canvas, args):
        """
        Modifies a shape on the canvas based on the given arguments.

        Parameters:
            canvas (Canvas): The canvas object on which the shape is drawn.
            args (list): The list of arguments specifying the modifications to be made.

        Returns:
            str: A message indicating the result of the modification.

        Raises:
            ValueError: If the shape ID is invalid.

        """
        if len(args) < 2:
            print(f"Invalid modify command: {args}")
            return "Invalid modify command"

        try:
            shape_id = self.selected_command_id
        except ValueError:
            print(f"Invalid shape ID: {args[0]}")
            return f"Invalid shape ID: {args[0]}"

        shape_type = canvas.type(shape_id)
        modifications = []
        current_mod = []

        for arg in args[1:]:
            if arg in ['colour', 'draw']:
                if current_mod:
                    modifications.append(current_mod)
                current_mod = [arg]
            else:
                current_mod.append(arg)
        
        if current_mod:
            modifications.append(current_mod)

        for mod in modifications:
            mod_type = mod[0]
            if mod_type == 'colour':
                if len(mod) != 4:
                    print(f"Invalid colour modification: {mod}")
                    continue
                r, g, b = map(int, mod[1:4])
                color = f"#{r:02x}{g:02x}{b:02x}"
                # Use 'fill' for lines and text, 'outline' for other shapes
                if shape_type in ["line", "text"]:
                    canvas.itemconfig(shape_id, fill=color)
                else:
                    canvas.itemconfig(shape_id, outline=color)
            elif mod_type == 'draw':
                if len(mod) != 5:
                    print(f"Invalid draw modification: {mod}")
                    continue
                x1, y1, x2, y2 = map(int, mod[1:5])
                canvas.coords(shape_id, x1, y1, x2, y2)

        print(f"Modified shape with ID: {shape_id}")
        return f"Modified shape with ID: {shape_id}"