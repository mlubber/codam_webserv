#!/usr/bin/env python3

import cgi

form = cgi.FieldStorage()
name = form.getvalue("name", "Stranger")
age = form.getvalue("age", "unknown")

print("Content-Type: text/html\r\n\r\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Greeting</title>
    <style>
        body {{ font-family: Verdana, sans-serif; background-color: #fffbe6; text-align: center; padding: 50px; }}
        h1 {{ color: #4CAF50; }}
    </style>
</head>
<body>
    <h1>Hello, {name}!</h1>
    <p>You are {age} years old.</p>
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
