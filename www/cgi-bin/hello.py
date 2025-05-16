#!/usr/bin/env python3
import cgi


# --- TESTING SIG INTERRUPTS
# import os
# import signal
# import time

# # Simulate some processing
# print("Starting CGI script processing...")
# time.sleep(2)  # Pretend to do some work for 2 seconds

# # Send SIGINT to itself
# print("Sending SIGINT to self...")
# os.kill(os.getpid(), signal.SIGINT)
# --- END OF TESTING SIG INTERRUPTS



# Tell the browser we are sending HTML
# print("Content-Type: text/html\n")

# # Get form data
# form = cgi.FieldStorage()
# name = form.getvalue("name", "Guest")  # Get 'name' from form, default to "Guest" if empty

# Generate response
print("Content-Type: text/html\n")
print("""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hello</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f0f8ff; text-align: center; padding: 50px; }
        h1 { color: #333; }
    </style>
</head>
<body>
    <h1>Hello from Python CGI!</h1>
    <p>This is a styled HTML page served via CGI.</p>
</body>
</html>
""")