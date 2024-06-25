import socket
import sys
import tkinter as tk
from canvas_app import CanvasApp

def process_commands(app, commands):
    for command in commands:
        try:
            response = app.execute_command(command)
            print(response)
        except Exception as e:
            print(f"An error occurred: {e}")

def set_nickname(self):
        while True:
            nickname = input("Enter your nickname: ").strip()
            if nickname:
                self.socket.sendall(f"NICKNAME {nickname}".encode())
                response = self.socket.recv(1024).decode().strip()
                if response == "NICKNAME_ACCEPTED":
                    print(f"Welcome, {nickname}!")
                    return nickname
                else:
                    print("Nickname already taken. Please choose another.")
            else:
                print("Nickname cannot be empty. Please try again.")

def main(commands=None):
    root = tk.Tk()
    app = CanvasApp(root)

    if commands:
        root.after(0, process_commands, app, commands)
    else:
        print("Connected to NetSketch server. Type 'help' for available commands.")
        def read_input():
            user_input = input("NetSketch> ").strip()
            if user_input.lower() == 'exit':
                print("Disconnecting from server...")
                root.quit()  # This will end the Tkinter main loop
            else:
                try:
                    response = app.execute_command(user_input)
                    print(response)
                except Exception as e:
                    print(f"An error occurred: {e}")
                root.after(0, read_input)  # Schedule next input read

        root.after(0, read_input)

    root.mainloop()

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == '--test':
        commands = sys.stdin.read().splitlines()
        main(commands)
    else:
        main()
