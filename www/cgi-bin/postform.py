#!/usr/bin/env python3

import cgi
import os
import sys

# Read the POST body from stdin
form = cgi.FieldStorage()
language = form.getvalue("language", "Unknown")
level = form.getvalue("level", "Not specified")

# Respond
print("Content-Type: text/html\r\n\r\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Post Response</title>
    <style>
        body {{ font-family: Arial, sans-serif; background-color: #eef; text-align: center; padding: 40px; }}
        h1 {{ color: #007BFF; }}
    </style>
</head>
<body>
    <h1>Thanks for your submission!</h1>
    <p>Your favorite language is <strong>{language}</strong>.</p>
    <p>Your skill level is <strong>{level}</strong>.</p>
    <a href="/" style="
        display: inline-block;
        margin-top: 20px;
        padding: 10px 20px;
        background-color: #4CAF50;
        color: white;
        text-decoration: none;
        border-radius: 5px;
    ">Back to Home</a>
</body>
</html>
""")
