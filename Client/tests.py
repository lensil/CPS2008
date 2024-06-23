import subprocess
import time
import socket
import unittest
import threading

class IntegrationTestSetup:
    def __init__(self, server_path, client_path):
        self.server_path = server_path
        self.client_path = client_path
        self.server_process = None

    def start_server(self):
        self.server_process = subprocess.Popen([self.server_path])
        time.sleep(2)  # Give the server some time to start

    def stop_server(self):
        if self.server_process:
            self.server_process.terminate()
            self.server_process.wait()

    def run_client(self, commands):
        print("Running client with commands:", commands)
        client_process = subprocess.Popen(['python3', self.client_path], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        stdout, stderr = client_process.communicate(input="\n".join(commands))
        print("Client stdout:", stdout)
        print("Client stderr:", stderr)
        return stdout, stderr
    


def run_client_with_output(commands):
    client_process = subprocess.Popen(['python3', '/Users/lenisesilvio/CPS2008-1/Client/client.py', '--test'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    stdout, stderr = client_process.communicate(input="\n".join(commands))
    return stdout, stderr

def test_broadcast_command(self):
    def run_first_client():
        stdout, stderr = run_client_with_output(["draw line 1 10 10 100 100 red"])
        return stdout, stderr

    def run_second_client():
        stdout, stderr = run_client_with_output([])
        return stdout, stderr

    thread1 = threading.Thread(target=run_first_client)
    thread2 = threading.Thread(target=run_second_client)

    thread1.start()
    time.sleep(1)  # Ensure the first client has time to send the command
    thread2.start()

    thread1.join()
    thread2.join()

    # Check that the second client received the command
    stdout, stderr = run_second_client()
    self.assertIn("draw line 1 10 10 100 100 red", stdout)


class IntegrationTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.test_setup = IntegrationTestSetup("/Users/lenisesilvio/CPS2008-1/build/test", "/Users/lenisesilvio/CPS2008-1/Client/client.py")
        cls.test_setup.start_server()

    @classmethod
    def tearDownClass(cls):
        cls.test_setup.stop_server()

    def test_connection(self):
        stdout, stderr = self.test_setup.run_client(["connect"])
        print("Test stdout:", stdout)
        print("Test stderr:", stderr)
        self.assertIn("Connected to server", stdout)

    def test_draw_command(self):
        stdout, stderr = self.test_setup.run_client(["draw line 1 10 10 100 100 red"])
        print("Test stdout:", stdout)
        print("Test stderr:", stderr)
        self.assertIn("Command processed successfully.", stdout)

    def test_delete_command(self):
        stdout, stderr = self.test_setup.run_client(["delete 1"])
        print("Test stdout:", stdout)
        print("Test stderr:", stderr)
        self.assertIn("Command processed successfully.", stdout)

    def test_broadcast_command(self):
        def run_first_client():
            stdout, stderr = run_client_with_output(["draw line 1 10 10 100 100 red"])
            return stdout, stderr

        def run_second_client():
            stdout, stderr = run_client_with_output([])
            return stdout, stderr

        thread1 = threading.Thread(target=run_first_client)
        thread2 = threading.Thread(target=run_second_client)

        thread1.start()
        time.sleep(1)  # Ensure the first client has time to send the command
        thread2.start()

        thread1.join()
        thread2.join()

        # Check that the second client received the command
        stdout, stderr = run_second_client()
        self.assertIn("draw line 1 10 10 100 100 red", stdout)

if __name__ == "__main__":
    unittest.main()