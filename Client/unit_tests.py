import unittest
from unittest.mock import MagicMock, patch
from commands import Commands
from canvas_app import CanvasApp

class TestCommands(unittest.TestCase):
    def setUp(self):
        self.commands = Commands()
        self.mock_canvas = MagicMock()

    def test_rgb_to_hex(self):
        self.assertEqual(self.commands.rgb_to_hex(255, 0, 0), '#ff0000')
        self.assertEqual(self.commands.rgb_to_hex(0, 255, 0), '#00ff00')
        self.assertEqual(self.commands.rgb_to_hex(0, 0, 255), '#0000ff')

    def test_apply_draw_command_line(self):
        command = "draw line 1 10 20 30 40 255 0 0"
        self.commands.apply_draw_command(self.mock_canvas, command)
        self.mock_canvas.create_line.assert_called_once_with(10, 20, 30, 40, fill='#ff0000')

    def test_apply_draw_command_rectangle(self):
        command = "draw rectangle 1 10 20 30 40 0 255 0"
        self.commands.apply_draw_command(self.mock_canvas, command)
        self.mock_canvas.create_rectangle.assert_called_once_with(10, 20, 30, 40, outline='#00ff00')

    def test_apply_draw_command_circle(self):
        command = "draw circle 1 10 20 30 40 0 0 255"
        self.commands.apply_draw_command(self.mock_canvas, command)
        self.mock_canvas.create_oval.assert_called_once_with(10, 20, 30, 40, outline='#0000ff')

    def test_apply_draw_command_text(self):
        command = "draw text 1 10 20 'Hello' 255 255 255"
        self.commands.apply_draw_command(self.mock_canvas, command)
        self.mock_canvas.create_text.assert_called_once_with(10, 20, text='Hello', fill='#ffffff')

    def test_delete_command(self):
        self.commands.delete_command(self.mock_canvas, 1)
        self.mock_canvas.delete.assert_called_once_with(1)

    def test_modify_command(self):
        self.commands.selected_command_id = 1
        self.mock_canvas.type.return_value = "line"  # Set the shape type to "line"
        args = ['colour', '255', '0', '0']
        self.commands.modify_command(self.mock_canvas, args)
        self.mock_canvas.itemconfig.assert_called_once_with(1, fill='#ff0000')

    def test_clear_all(self):
        self.commands.shapes = {1: "shape1", 2: "shape2", 3: "shape3"}
        self.commands.draw_commands = [(1, "cmd1"), (2, "cmd2"), (3, "cmd3")]
        self.commands.user_commands = {1, 2}

        self.commands.apply_draw_command(self.mock_canvas, "clear all")

        self.mock_canvas.delete.assert_called_once_with("all")
        self.assertEqual(self.commands.shapes, {})
        self.assertEqual(self.commands.draw_commands, [])
        self.assertEqual(self.commands.user_commands, set())

    def test_clear_mine(self):
        self.commands.shapes = {1: "shape1", 2: "shape2", 3: "shape3"}
        self.commands.draw_commands = [(1, "cmd1"), (2, "cmd2"), (3, "cmd3")]
        self.commands.user_commands = {1, 2}

        self.commands.apply_draw_command(self.mock_canvas, "clear mine 1 2")

        self.mock_canvas.delete.assert_any_call(1)
        self.mock_canvas.delete.assert_any_call(2)
        self.assertEqual(self.commands.shapes, {3: "shape3"})
        self.assertEqual(self.commands.draw_commands, [(3, "cmd3")])
        self.assertEqual(self.commands.user_commands, set())

    def test_list_commands_all(self):
        self.commands.draw_commands = [
            (1, "draw line 1 10 20 30 40 255 0 0"),
            (2, "draw rectangle 2 50 60 70 80 0 255 0"),
            (3, "draw circle 3 90 100 110 120 0 0 255")
        ]
        self.commands.user_commands = {1, 2, 3}

        result = self.commands.list_commands(filter_tool="all", filter_user="all")
        self.assertEqual(len(result), 3)
        self.assertIn((1, "draw line 1 10 20 30 40 255 0 0"), result)
        self.assertIn((2, "draw rectangle 2 50 60 70 80 0 255 0"), result)
        self.assertIn((3, "draw circle 3 90 100 110 120 0 0 255"), result)

    def test_list_commands_filter_tool(self):
        self.commands.draw_commands = [
            (1, "draw line 1 10 20 30 40 255 0 0"),
            (2, "draw rectangle 2 50 60 70 80 0 255 0"),
            (3, "draw circle 3 90 100 110 120 0 0 255")
        ]
        self.commands.user_commands = {1, 2, 3}

        result = self.commands.list_commands(filter_tool="line", filter_user="all")
        self.assertEqual(len(result), 1)
        self.assertIn((1, "draw line 1 10 20 30 40 255 0 0"), result)

    def test_list_commands_filter_user(self):
        self.commands.draw_commands = [
            (1, "draw line 1 10 20 30 40 255 0 0"),
            (2, "draw rectangle 2 50 60 70 80 0 255 0"),
            (3, "draw circle 3 90 100 110 120 0 0 255")
        ]
        self.commands.user_commands = {1, 2}

        result = self.commands.list_commands(filter_tool="all", filter_user="mine")
        self.assertEqual(len(result), 2)
        self.assertIn((1, "draw line 1 10 20 30 40 255 0 0"), result)
        self.assertIn((2, "draw rectangle 2 50 60 70 80 0 255 0"), result)

class TestCanvasApp(unittest.TestCase):
    @patch('socket.socket')
    def setUp(self, mock_socket):
        self.root = MagicMock()
        self.app = CanvasApp(self.root)
        self.app.client_socket = mock_socket

    def test_execute_command_tool(self):
        self.app.execute_command("tool line")
        self.assertEqual(self.app.current_tool, "line")

    def test_execute_command_colour(self):
        self.app.execute_command("colour 255 0 0")
        self.assertEqual(self.app.current_color, "255 0 0")

    def test_execute_command_draw(self):
        self.app.current_tool = "line"
        self.app.current_color = "255 0 0"
        with patch.object(self.app, 'draw_shape') as mock_draw:
            self.app.execute_command("draw 10 20 30 40")
            mock_draw.assert_called_once_with("line", 10, 20, 30, 40, "255 0 0")

    def test_execute_command_list(self):
        self.app.execute_command("list all all")
        self.app.client_socket.sendall.assert_called_once_with(b"list all all\n")

    def test_execute_command_delete(self):
        self.app.execute_command("delete 1")
        self.app.client_socket.sendall.assert_called_once_with(b"delete 1\n")

    @patch('socket.socket')
    def test_execute_command_clear_all(self, mock_socket):
        app = CanvasApp(MagicMock())
        app.canvas = MagicMock()
        app.user_commands = {1, 2, 3}
        
        app.execute_command("clear all")
        
        app.canvas.delete.assert_called_once_with("all")
        self.assertEqual(app.user_commands, set())
        app.client_socket.sendall.assert_called_once_with(b"clear all\n")

    @patch('socket.socket')
    def test_execute_command_clear_mine(self, mock_socket):
        app = CanvasApp(MagicMock())
        app.canvas = MagicMock()
        app.user_commands = {1, 2, 3}
        
        app.execute_command("clear mine")
        
        for shape_id in {1, 2, 3}:
            app.canvas.delete.assert_any_call(shape_id)
        self.assertEqual(app.user_commands, set())
        app.client_socket.sendall.assert_called_once()  # Check that sendall was called

    @patch('socket.socket')
    def test_execute_command_list(self, mock_socket):
        app = CanvasApp(MagicMock())
        
        app.execute_command("list all all")
        
        app.client_socket.sendall.assert_called_once_with(b"list all all\n")

if __name__ == '__main__':
    unittest.main()