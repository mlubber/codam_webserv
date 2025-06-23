#!/usr/bin/env python3

import cgi

print("Content-Type: text/html\r\n\r\n")

form = cgi.FieldStorage()
name = form.getvalue("name")

if name:
    response = f"<h2>Hi, {name}!</h2>"
else:
    response = """
    <form method="post" action="/cgi-bin/form.py">
        <label for="name">Enter your name:</label><br><br>
        <input type="text" name="name" required />
        <br><br>
        <input type="submit" value="Submit" />
    </form>
    """

print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Form Example</title>
    <style>
        body {{ font-family: Tahoma, sans-serif; background-color: #e6f7ff; text-align: center; padding: 40px; }}
        form {{ display: inline-block; background: #fff; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px #ccc; }}
        input[type="text"] {{ padding: 8px; width: 200px; }}
        input[type="submit"] {{ padding: 8px 16px; }}
        h2 {{ color: #007acc; }}
    </style>
</head>
<body>
    {response}
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
