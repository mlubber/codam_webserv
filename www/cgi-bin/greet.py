#!/usr/bin/env python3

import cgi

form = cgi.FieldStorage()
name = form.getvalue("name", "Stranger")

print("Content-Type: text/html\n")
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
    <p>This greeting is dynamic and styled.</p>
</body>
</html>
""")
