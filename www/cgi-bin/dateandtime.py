#!/usr/bin/env python3

from datetime import datetime

now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

print("Content-Type: text/html\n")
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
</body>
</html>
""")
