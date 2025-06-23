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
print("Content-Type: text/html\r\n\r\n")  # Corrected header termination
print("""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Hello CGI</title>
	<style>
		body {
			font-family: Arial, sans-serif;
			margin: 0;
			padding: 0;
			background-color: #ffffff;
			color: #e0e0e0;
			text-align: center;
		}
		.container {
			max-width: 800px;
			margin: 50px auto;
			padding: 20px;
			background: #90EE90;
			box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.5);
			border-radius: 8px;
		}
		h1 {
			color: #000000;
		}
		p {
			color: #000000;
			font-size: 18px;
		}
		.btn {
			display: inline-block;
			padding: 10px 20px;
			margin-top: 20px;
			font-size: 18px;
			color: white;
			background: #4CAF50;
			border: none;
			border-radius: 5px;
			text-decoration: none;
			transition: background 0.3s;
		}
		.btn:hover {
			background: #347836;
		}
	</style>
</head>
<body>
	<div class="container">
		<h1>Hello from Python CGI!</h1>
		<p>This is a styled HTML page served via CGI.</p>
		<a href="javascript:history.back()" class="btn">Go Back</a>
		<a href="/" class="btn">Home</a>
	</div>
</body>
</html>
""")