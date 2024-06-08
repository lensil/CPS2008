class Commands:
    def __init__(self):
        self.draw_commands = []
        self.command_id = 0
        self.selected_command_id = None
    
    def apply_draw_command(self, canvas, command, redraw=False):
        parts = command.split()
        if parts[0] == "draw":
            shape = parts[1]
            if shape == "line":
                x1, y1, x2, y2, color = int(parts[2]), int(parts[3]), int(parts[4]), int(parts[5]), parts[6]
                canvas.create_line(x1, y1, x2, y2, fill=color)
            elif shape == "rectangle":
                x1, y1, x2, y2, color = int(parts[2]), int(parts[3]), int(parts[4]), int(parts[5]), parts[6]
                canvas.create_rectangle(x1, y1, x2, y2, outline=color)
            elif shape == "circle":
                x1, y1, x2, y2, color = int(parts[2]), int(parts[3]), int(parts[4]), int(parts[5]), parts[6]
                canvas.create_oval(x1, y1, x2, y2, outline=color)
            elif shape == "text":
                x, y, text, color = int(parts[2]), int(parts[3]), ' '.join(parts[4:-1]), parts[-1]
                canvas.create_text(x, y, text=text, fill=color)
            
            if not redraw:
                self.draw_commands.append((self.command_id, command))
                self.command_id += 1
    
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