<?php
session_start();

$_SESSION = array(); // Clear the session variables

session_destroy();

header("location: index.php"); // Redirect to the login page
?>