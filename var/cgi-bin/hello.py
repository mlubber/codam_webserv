#!/usr/bin/env python3
import cgi

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