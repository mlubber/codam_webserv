#!/usr/bin/env python3
import cgi
import time


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
print("Content-Type: text/html\r\n\r\n")  # Corrected header termination
time.sleep(15)
print("""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Hello</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #90EE90; text-align: center; padding: 50px; }
        h1 { color: #333; }
    </style>
</head>
<body>
    <h1>This Script is supposed to time out and not viewable!</h1>
	<a href="/" style="
		display: inline-block;
		margin-top: 20px;
		padding: 10px 20px;
		background-color: #4CAF50;
		color: white;
		text-decoration: none;
		border-radius: 5px;
		font-weight: bold;
	">Back to Home</a>
</body>
</html>
""")
