import unittest
import subprocess
import time
import socket
import threading
import sys
import os
import signal

# Add the parent directory of the current script to the Python path
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
project_root = os.path.dirname(parent_dir)
sys.path.insert(0, parent_dir)

from commands import Commands
from canvas_app import CanvasApp

class IntegrationTestSetup:
    def __init__(self, server_path, client_path):
        self.server_path = "/Users/lenisesilvio/CPS2008-1/build/server"
        self.client_path = "/Users/lenisesilvio/CPS2008-1/Client/client.py"
        self.server_process = None

    def start_server(self):
        print(f"Attempting to start server at: {self.server_path}")
        if not os.path.exists(self.server_path):
            print(f"Error: Server executable not found at {self.server_path}")
            print(f"Current working directory: {os.getcwd()}")
            print(f"Directory contents: {os.listdir(os.path.dirname(self.server_path))}")
            raise FileNotFoundError(f"Server executable not found at {self.server_path}")
        
        try:
            self.server_process = subprocess.Popen([self.server_path])
            print(f"Server process started with PID: {self.server_process.pid}")
            time.sleep(2)  # Give the server some time to start
        except Exception as e:
            print(f"Error starting server: {str(e)}")
            raise

    def stop_server(self):
        if self.server_process:
            print(f"Stopping server process with PID: {self.server_process.pid}")
            self.server_process.terminate()
            try:
                self.server_process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                print("Server didn't terminate gracefully, forcing...")
                self.server_process.kill()
            print("Server stopped")

    def run_client(self, commands, timeout=10):
        print(f"Running client with commands: {commands}")
        print(f"Client path: {self.client_path}")
        if not os.path.exists(self.client_path):
            print(f"Error: Client script not found at {self.client_path}")
            print(f"Directory contents: {os.listdir(os.path.dirname(self.client_path))}")
            raise FileNotFoundError(f"Client script not found at {self.client_path}")

        try:
            client_process = subprocess.Popen(['python3', self.client_path, '--test'], 
                                              stdin=subprocess.PIPE, 
                                              stdout=subprocess.PIPE, 
                                              stderr=subprocess.PIPE, 
                                              text=True)
            try:
                stdout, stderr = client_process.communicate(input="\n".join(commands) + "\nexit\n", timeout=timeout)
            except subprocess.TimeoutExpired:
                print(f"Client process timed out after {timeout} seconds")
                client_process.kill()
                stdout, stderr = client_process.communicate()
            print(f"Client stdout: {stdout}")
            print(f"Client stderr: {stderr}")
            return stdout, stderr
        except Exception as e:
            print(f"Error running client: {str(e)}")
            raise

class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.server_path = os.path.join(project_root, "build", "test")
        cls.client_path = os.path.join(parent_dir, "client.py")
        
        print(f"Server path: {cls.server_path}")
        print(f"Client path: {cls.client_path}")
        
        cls.test_setup = IntegrationTestSetup(cls.server_path, cls.client_path)
        cls.test_setup.start_server()

    @classmethod
    def tearDownClass(cls):
        cls.test_setup.stop_server()

    def test_connection(self):
        # The client doesn't have a "connect" command, so we'll just check if it starts without error
        stdout, stderr = self.test_setup.run_client([""])
        self.assertNotIn("Error", stdout + stderr)

    def test_draw_command(self):
        commands = [
            "tool line",
            "colour 255 0 0",
            "draw 10 10 100 100"
        ]
        stdout, stderr = self.test_setup.run_client(commands)
        self.assertIn("Command processed successfully", stdout + stderr)

    def test_list_command(self):
        commands = [
            "tool line",
            "colour 255 0 0",
            "draw 10 10 100 100",
            "tool rectangle",
            "colour 0 255 0",
            "draw 20 20 200 200",
            "list all all"
        ]
        stdout, stderr = self.test_setup.run_client(commands)
        self.assertIn("line", stdout + stderr)
        self.assertIn("rectangle", stdout + stderr)

    def test_modify_command(self):
        commands = [
            "tool line",
            "colour 255 0 0",
            "draw 10 10 100 100",
            "select 1",
            "modify colour 0 0 255",
            "list all all"
        ]
        stdout, stderr = self.test_setup.run_client(commands)
        self.assertIn("0 0 255", stdout + stderr)

    def test_delete_command(self):
        commands = [
            "tool line",
            "colour 255 0 0",
            "draw 10 10 100 100",
            "delete 1",
            "list all all"
        ]
        stdout, stderr = self.test_setup.run_client(commands)
        self.assertNotIn("line 10 10 100 100 255 0 0", stdout + stderr)

    def test_clear_all_command(self):
        commands = [
            "tool line",
            "colour 255 0 0",
            "draw 10 10 100 100",
            "tool rectangle",
            "colour 0 255 0",
            "draw 20 20 200 200",
            "list all all",
            "clear all",
            "list all all"
        ]
        stdout, stderr = self.test_setup.run_client(commands)
        print("Full output:")
        print(stdout)
        print("Error output:")
        print(stderr)
        
        # Check that shapes were drawn before clearing
        self.assertIn("[1] =>", stdout)
        self.assertIn("[2] =>", stdout)
        
        # Check if the clear command was processed
        self.assertIn("All shapes cleared from the canvas", stdout, "The 'clear all' command was not processed correctly")

if __name__ == "__main__":
    unittest.main()