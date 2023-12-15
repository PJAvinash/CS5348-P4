import os
import subprocess

# Specify the path to the folder containing files
folder_path = 'tests/'

# Get a list of all files in the folder
files = [f for f in os.listdir(folder_path) if os.path.isfile(os.path.join(folder_path, f))]

# Run ./fcheck on each file
for file in files:
    file_path = os.path.join(folder_path, file)
    
    # Construct the command to run ./fcheck on the current file
    command = ['./fcheck', file_path]

    # Run the command
    try:
        print(command[1])
        subprocess.run(command, check=True)
        print("\n")
    except subprocess.CalledProcessError as e:
        #print(f"Error running ./fcheck on {file_path}: {e}")
        a = 1
