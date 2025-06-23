#!/usr/bin/env python3

from datetime import datetime

now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

print("Content-Type: text/html\r\n\r\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Server Time</title>
	<style>
		body {{
			font-family: Arial, sans-serif;
			margin: 0;
			padding: 0;
			background-color: #ffffff;
			color: #e0e0e0;
			text-align: center;
		}}
		.container {{
			max-width: 800px;
			margin: 50px auto;
			padding: 20px;
			background: #90EE90;
			box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.5);
			border-radius: 8px;
		}}
		.time-box {{
			display: inline-block;
			background: #fff3cd;
			padding: 20px 40px;
			border-radius: 12px;
			box-shadow: 0 0 8px rgba(0,0,0,0.1);
			font-size: 1.5em;
		}}
		h1 {{
			color: #000000;
		}}
		p {{
			color: #000000;
			font-size: 18px;
		}}
		.btn {{
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
		}}
		.btn:hover {{
			background: #347836;
		}}
	</style>
</head>
<body>
	<div class="container">
		<p>Current Server Time: </p>
		<h1><strong>{now}</strong></h1>
		<a href="javascript:history.back()" class="btn">Go Back</a>
		<a href="/" class="btn">Home</a>
	</div>
</body>
</html>
""")
