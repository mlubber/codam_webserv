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
print(f"""
<html>
<head><title>CGI Response</title></head>
<body>
    <h1>Hello, Billy!</h1>
    <p>Thank you for using CGI!</p>
</body>
</html>
""")