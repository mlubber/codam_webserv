#!/usr/bin/php-cgi
<?php
// Output CGI-compliant HTTP headers
header("Content-Type: text/html");

echo '<!DOCTYPE html>
<html lang="en">
<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>CGI: PHP - Webserv</title>
	<style>
		body {
			font-family: Arial, sans-serif;
			margin: 0;
			padding: 0;
			background-color: #121212;
			color: #e0e0e0;
			text-align: center;
		}
		.container {
			max-width: 800px;
			margin: 50px auto;
			padding: 20px;
			background:rgb(203, 144, 238);
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
			background:rgb(145, 76, 175);
			border: none;
			border-radius: 5px;
			text-decoration: none;
			transition: background 0.3s;
		}
		.btn:hover {
			background:rgb(119, 58, 154);
		}
	</style>
</head>
<body>
	<div class="container">
		<h1>Hello from PHP CGI!</h1>
		<p>Current time: ' . date("Y-m-d H:i:s") . '</p>
		<p>PHP Version: ' . phpversion() . '</p>
		<a href="javascript:history.back()" class="btn">Go Back</a>
		<a href="/" class="btn">Home</a>
	</div>
</body>
</html>';
?>