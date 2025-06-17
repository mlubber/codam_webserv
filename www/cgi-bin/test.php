#!/usr/bin/php-cgi
<?php
// Output CGI-compliant HTTP headers
header("Content-Type: text/html");

echo "<!DOCTYPE html>";
echo "<html><head><title>PHP CGI Test</title></head><body>";
echo "<h1>PHP via CGI is working!</h1>";
echo "<p>Current time: " . date("Y-m-d H:i:s") . "</p>";
echo "<p>PHP Version: " . phpversion() . "</p>";

// Add a button that redirects to the homepage
echo '<form action="/" method="get">';
echo '<button type="submit">Back to Homepage</button>';
echo '</form>';

echo "</body></html>";
?>
