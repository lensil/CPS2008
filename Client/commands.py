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
        print(f"Part 0: {parts[0]}")
        if parts[0] == "delete":
            shape_id = int(parts[1])
            self.delete_command(canvas, shape_id)
            return
        elif parts[0] == "clear":
            if parts[1] == "all":
                canvas.delete("all")
                self.shapes = {}
                self.draw_commands = []
                self.command_id = 0
                self.selected_command_id = None
                self.user_commands = set() 
                return   
            elif parts[1] == "mine":
                self.redraw(canvas, filter_user="mine")
                return
        elif parts[0] == "undo":
            self.undo_last(canvas)
            return
        elif parts[0] == "modify":
            self.handle_modify_command(canvas, parts[1:])
            return
        if len(parts) < 7:
            print(f"Invalid command format or missing arguments: '{command}'")
            return

        shape = parts[1]
        try:
            if shape == "text":
                shape_id, x, y = map(int, parts[2:5])
                text = ' '.join(parts[5:-3]).strip("'")
                r, g, b = map(int, parts[-3:])
                color = f"#{r:02x}{g:02x}{b:02x}"
                shape_id = canvas.create_text(x, y, text=text, fill=color)
            else:
                shape_id, x1, y1, x2, y2 = map(int, parts[2:7])
                r, g, b = map(int, parts[7:10])
                color = f"#{r:02x}{g:02x}{b:02x}"

                if shape == "line":
                    shape_id = canvas.create_line(x1, y1, x2, y2, fill=color)
                elif shape == "rectangle":
                    shape_id = canvas.create_rectangle(x1, y1, x2, y2, outline=color)
                elif shape == "circle":
                    shape_id = canvas.create_oval(x1, y1, x2, y2, outline=color)
                else:
                    print(f"Unsupported shape type: '{shape}' in command: '{command}'")
                    return

            print(f"Created {shape} with ID: {shape_id}")
            print(f"Shapes array: {self.shapes}")
            print(f"Draw commands array: {self.draw_commands}")

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
        self.user_commands.add(shape_id)

    def list_commands(self, filter_tool=None, filter_user=None):
        filtered_commands = []
        for shape_id, command in self.draw_commands:
            parts = command.split()
            shape_type = parts[1]
            if filter_tool and shape_type != filter_tool:
                continue
            if filter_user == "mine" and shape_id not in self.user_commands:
                continue
            color = f"[{parts[-3]} {parts[-2]} {parts[-1]}]"
            if shape_type == 'text':
                text = ' '.join(parts[5:-3]).strip("'")
                coords = f"[{parts[3]} {parts[4]}]"
                formatted_command = f"[{shape_id}] => [{shape_type}] {color} {coords} *\"{text}\"*"
            else:
                coords = f"[{parts[3]} {parts[4]} {parts[5]} {parts[6]}]"
                formatted_command = f"[{shape_id}] => [{shape_type}] {color} {coords}"
            filtered_commands.append(formatted_command)
        return filtered_commands
    
    def select_command(self, command_id):
        if command_id in self.shapes:
            self.selected_command_id = command_id
            return f"Selected command with ID: {command_id}"
        else:
            return f"No command found with ID: {command_id}"

    def modify_command(self, canvas, modifications):
        if self.selected_command_id is None:
            return "No command selected. Use 'select' command first."

        old_command = self.shapes[self.selected_command_id]
        parts = old_command.split()
        shape_type = parts[1]

        for mod in modifications:
            if mod[0] == 'colour':
                r, g, b = map(int, mod[1:])
                color = f"#{r:02x}{g:02x}{b:02x}"
                if shape_type == 'text':
                    canvas.itemconfig(self.selected_command_id, fill=color)
                else:
                    canvas.itemconfig(self.selected_command_id, outline=color)
                parts[-3:] = [str(r), str(g), str(b)]
            elif mod[0] == 'draw':
                if shape_type == 'text':
                    x, y = map(int, mod[1:3])
                    text = ' '.join(mod[3:]).strip('"*')
                    canvas.coords(self.selected_command_id, x, y)
                    canvas.itemconfig(self.selected_command_id, text=text)
                    parts[3:5] = [str(x), str(y)]
                    parts[5] = f"'{text}'"
                else:
                    x1, y1, x2, y2 = map(int, mod[1:5])
                    canvas.coords(self.selected_command_id, x1, y1, x2, y2)
                    parts[3:7] = [str(x1), str(y1), str(x2), str(y2)]

        new_command = ' '.join(parts)
        self.shapes[self.selected_command_id] = new_command
        self.draw_commands = [(id, cmd) if id != self.selected_command_id else (id, new_command) for id, cmd in self.draw_commands]
        
        modified_id = self.selected_command_id
        self.selected_command_id = None
        return f"Modified command with ID: {modified_id}"

    def list_commands(self, filter_tool=None, filter_user=None):
        filtered_commands = []
        for shape_id, command in self.draw_commands:
            parts = command.split()
            shape_type = parts[1]
            if filter_tool and shape_type != filter_tool:
                continue
            if filter_user == "mine" and shape_id not in self.user_commands:
                continue
            color = f"[{parts[-3]} {parts[-2]} {parts[-1]}]"
            if shape_type == 'text':
                text = ' '.join(parts[5:-3]).strip("'")
                coords = f"[{parts[3]} {parts[4]}]"
                formatted_command = f"[{shape_id}] => [{shape_type}] {color} {coords} *\"{text}\"*"
            else:
                coords = f"[{parts[3]} {parts[4]} {parts[5]} {parts[6]}]"
                formatted_command = f"[{shape_id}] => [{shape_type}] {color} {coords}"
            filtered_commands.append(formatted_command)
        return filtered_commands
    
    def delete_command(self, canvas, shape_id):
        print(f"Deleting shape with ID: {shape_id}")
        print(self.shapes)
        canvas.delete(shape_id)
        #elf.draw_commands = [cmd for cmd in self.draw_commands if cmd[0] != shape_id]
        #self.draw_commands = [cmd for cmd in self.draw_commands if cmd[0] != shape_id]
        #if shape_id in self.shapes:
            #del self.shapes[shape_id] 
    
    def undo_last(self, canvas):
        if self.draw_commands:
            last_command = self.draw_commands.pop()
            if last_command[0] in self.shapes:
                del self.shapes[last_command[0]]
            if last_command[0] in self.user_commands:
                self.user_commands.remove(last_command[0])
            canvas.delete(last_command[0])

    def redraw(self, canvas, filter_user=None):
        canvas.delete("all")
        new_shapes = {}
        id_mapping = {}
        for old_shape_id, command in self.draw_commands:
            new_shape_id = self.apply_draw_command(canvas, command, redraw=True)
            new_shapes[new_shape_id] = command
            id_mapping[old_shape_id] = new_shape_id

        self.shapes = new_shapes
        self.update_draw_commands(id_mapping)

    def update_draw_commands(self, id_mapping):
        updated_draw_commands = []
        for old_shape_id, command in self.draw_commands:
            new_shape_id = id_mapping.get(old_shape_id)
            if new_shape_id:
                updated_draw_commands.append((new_shape_id, command))
        self.draw_commands = updated_draw_commands

    def modify_command(self, canvas, args):
        if self.selected_command_id is None:
            return "No command selected. Use 'select' command first."

        return self.handle_modify_command(canvas, [str(self.selected_command_id)] + args)

    def handle_modify_command(self, canvas, args):
        if len(args) < 2:
            print(f"Invalid modify command: {args}")
            return "Invalid modify command"

        try:
            shape_id = int(args[0])
        except ValueError:
            print(f"Invalid shape ID: {args[0]}")
            return f"Invalid shape ID: {args[0]}"

        if shape_id not in self.shapes:
            print(f"Shape with ID {shape_id} not found.")
            return f"Shape with ID {shape_id} not found."

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
                canvas.itemconfig(shape_id, outline=color)
            elif mod_type == 'draw':
                if len(mod) != 5:
                    print(f"Invalid draw modification: {mod}")
                    continue
                x1, y1, x2, y2 = map(int, mod[1:5])
                canvas.coords(shape_id, x1, y1, x2, y2)

        # Update the command in the shapes dictionary
        old_command = self.shapes[shape_id].split()
        old_command[0] = 'modify'  # Change 'draw' to 'modify'
        old_command[1] = str(shape_id)  # Ensure shape_id is correct
        new_command = ' '.join(old_command[:2] + args[1:])  # Combine old command start with new modifications
        self.shapes[shape_id] = new_command

        # Update the corresponding entry in draw_commands
        for i, (id, cmd) in enumerate(self.draw_commands):
            if id == shape_id:
                self.draw_commands[i] = (shape_id, new_command)
                break

        print(f"Modified shape with ID: {shape_id}")
        return f"Modified shape with ID: {shape_id}"

# To do: fix issue with reconnecting

# To do: fix list

# To do: do clear function for "mine"

# To do: add command line connection in client 