#!/usr/bin/env python3

from datetime import datetime

now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

print("Content-Type: text/html\r\n\r\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Server Time</title>
    <style>
        body {{ font-family: 'Segoe UI', sans-serif; background-color: #fdf6e3; text-align: center; padding: 60px; }}
        .time-box {{
            display: inline-block;
            background: #fff3cd;
            padding: 20px 40px;
            border-radius: 12px;
            box-shadow: 0 0 8px rgba(0,0,0,0.1);
            font-size: 1.5em;
        }}
    </style>
</head>
<body>
    <div class="time-box">
        <p>Current Server Time:</p>
        <strong>{now}</strong>
    </div>
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
