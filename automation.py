import subprocess

# Define test cases (replace these with your actual test cases)
test_cases = {
    'addronce': 'ERROR: direct address used more than once.',
    'addronce2': 'ERROR: indirect address used more than once.',
    'badaddr': 'ERROR: bad direct address in inode.',
    'badfmt': 'ERROR: directory not properly formatted.',
    'badindir1': 'ERROR: bad indirect address in inode.',
    'badindir2': 'ERROR: bad indirect address in inode.',
    'badinode': 'ERROR: bad inode.',
    'badlarge': 'ERROR: indirect address used more than once.',
    'badrefcnt': 'ERROR: inode marked use but not found in a directory.',
    'badrefcnt2': 'ERROR: inode referred to in directory but marked free.',
    'badroot': 'ERROR: root directory does not exist.',
    'badroot2': 'ERROR: directory not properly formatted.',
    'dironce': 'ERROR: directory appears more than once in file system.',
    'good': '',
    'goodlarge': '',
    'goodlink': '',
    'goodrefcnt': '',
    'goodrm': '',
    'imrkfree': 'ERROR: address used by inode but marked free in bitmap.',
    'imrkused': 'ERROR: inode marked used, but not referenced in a directory.',
    'indirfree': 'ERROR: bitmap marks block in use but it is not in use.',
    'mismatch': 'ERROR: directory not properly formatted.',
    'mrkfree': 'ERROR: bitmap marks block in use but it is not in use.',
    'mrkused': 'ERROR: bitmap marks block in use but it is not in use.'
}

# Specify the path to your C program
c_program_path = './fcheck'

# Initialize counter for passed tests
passed_tests = 0

# Initialize counter for passed tests
passed_tests = 0

# Iterate through test cases
for test_file, expected_error in test_cases.items():
    # Construct the command to run your C program with the test file
    command = [c_program_path, f'tests/{test_file}']

    try:
        result = subprocess.run(command, capture_output=True, text=True, check=True)
        actual_error = result.stderr.strip()

        # Print the output for both successful and failed tests
        print(f'Test: {test_file}')
        print(f'Expected Error: {expected_error}')

        # Compare actual and expected error messages
        if actual_error == expected_error:
            print('Test passed\n')
            passed_tests += 1
        else:
            print(f'Test failed\nActual Error: {actual_error}\n')
    except subprocess.CalledProcessError as e:
        # Print the output for failed tests
        print(f'Test: {test_file}')
        print(f'Expected Error: {expected_error}')
        print(f'Error running test: {e.stderr}\n')

# Print the total number of passed tests
print(f'Total tests passed: {passed_tests}')