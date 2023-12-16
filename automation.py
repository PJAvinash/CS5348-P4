import subprocess

# Define test cases (replace these with your actual test cases)
test_cases= {
    'addronce': 'ERROR: direct address used more than once.',
    'addronce2': 'ERROR: indirect address used more than once.',
    'badaddr': 'ERROR: bad direct address in inode.',
    'badfmt': 'ERROR: directory not properly formatted.',
    'badindir1': 'ERROR: bad indirect address in inode.',
    'badindir2': 'ERROR: bad indirect address in inode.',
    'badinode': 'ERROR: bad inode.',
    'badlarge': 'ERROR: directory appears more than once in file system.',
    'badrefcnt': 'ERROR: bad reference count for file.',
    'badrefcnt2': 'ERROR: bad reference count for file.',
    'badroot': 'ERROR: root directory does not exist.',
    'badroot2': 'ERROR: root directory does not exist.',
    'dironce': 'ERROR: directory appears more than once in file system.',
    'good': '',
    'goodlarge': '',
    'goodlink': '',
    'goodrefcnt': '',
    'goodrm': '',
    'imrkfree': 'ERROR: inode referred to in directory but marked free.',
    'imrkused': 'ERROR: inode marked use but not found in a directory.',
    'indirfree': 'ERROR: address used by inode but marked free in bitmap.',
    'mismatch': 'ERROR: directory not properly formatted.',
    'mrkfree': 'ERROR: address used by inode but marked free in bitmap.',
    'mrkused': 'ERROR: bitmap marks block in use but it is not in use.'
}

# Specify the path to your C program
c_program_path = './fcheck'

# Initialize counter for passed tests
passed_tests = 0

# Iterate through test cases
for test_file, expected_error in test_cases.items():
    # Construct the command to run your C program with the test file
    command = [c_program_path, f'tests/{test_file}']

    # Run the command
    result = subprocess.run(command, capture_output=True, text=True)

    # Check if the program exited with a non-zero status code
    if result.returncode == 0:
        # Print the output for failed tests
        
        if expected_error.strip() == result.stderr.strip():
            passed_tests += 1
        else:
            print(f'Test: {test_file}')
            print(f'Expected Error: {expected_error} Actual Error: "{result.stderr}"')

    else:
        # Check if the actual error matches the expected error after trimming whitespaces
        actual_error = result.stderr.strip()
        if expected_error.strip() == actual_error.strip():
            print(f'Test passed for {test_file}\n')
            passed_tests += 1
        else:
            # Print the output for failed tests with additional information
            print(f'Test: {test_file}')
            print(f'Expected Error: "{expected_error}" Actual Error: "{actual_error}"')

# Print the total number of passed tests
print(f'Total tests passed: {passed_tests} in {len(test_cases.items())}')