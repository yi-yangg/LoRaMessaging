<?php
session_start();

require_once("settings.php");

// Sanitise user input
function sanitise($data)
{
	$data = trim($data);
	$data = stripslashes($data);
	$data = htmlspecialchars($data);
	return $data;
}

// Store error message
$errMsg = "";

// Redirect users to send.php if login
if (isset($_SESSION["id"])) {
	header("location:send.php");
	exit("Redirect");
}

// Check input data
if (isset($_POST["inputID"])) {
	$conn = NULL;

	// Connect to DB
	try {
		$conn = mysqli_connect($host, $user, $pwd, $sql_db);
	} catch (mysqli_sql_exception $e) {
		// handle the error
	}

	// Get input value from the form
	$id = sanitise($_POST["inputID"]);
	$password = hash("sha256", sanitise($_POST["inputPassword"])); // Hash input password

	// Check DB connection
	if (!$conn) {
		$errMsg .= "<p>Database Connection Failure</p>";
	} else {
		// Create user table if no exists
		$query = "CREATE TABLE IF NOT EXISTS user (
				id VARCHAR(255) PRIMARY KEY,
				password VARCHAR(255) NOT NULL,
				name VARCHAR(255) NOT NULL,
				created_at DATE DEFAULT (CURRENT_DATE()),
				updated_at DATE DEFAULT (CURRENT_DATE())
			);";

		$result = mysqli_query($conn, $query);

		$query = "SELECT * FROM user WHERE id = '$id' AND password = '$password';";

		$result = mysqli_query($conn, $query);

		// Check if match any id and password in the database
		if (mysqli_num_rows($result) == 0) {
			$errMsg .= "<p>Incorrect ID or Password</p>";
		} else {
			$row = mysqli_fetch_assoc($result);

			// Store id, password, and name into session variables
			$_SESSION["id"] = $id;
			$_SESSION["password"] = $password;
			$_SESSION["name"] = $row["name"];

			// Redirect to the send page
			header("location:send.php");
			exit("Redirect");
		}
		mysqli_close($conn);
	}
}
?>
<!DOCTYPE html>
<html lang="en">

<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Login | LoRa Project</title>
	<!-- Include Bootstrap framework -->
	<link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet"
		integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
	<script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"
		integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz"
		crossorigin="anonymous"></script>
</head>

<body>
	<!-- Include header -->
	<?php
	include_once("headerBeforeLogin.inc");
	?>

	<div class="container-fluid text-center py-5 d-flex flex-column align-items-center mx-auto">
		<!-- Heading -->
		<h1 class="display-3">LoRa Messaging System</h1>
		<p class="lead fs-2">Login</p>

		<div class="pt-5 w-25">
			<!-- User input form -->
			<form method="post" action="index.php">
				<div class="mb-3 text-start">
					<label for="inputID" class="form-label">ID</label>
					<input type="text" class="form-control" id="inputID" name="inputID" maxlength="12" required>
				</div>
				<div class="mb-3 text-start">
					<label for="inputPassword" class="form-label">Password</label>
					<input type="password" class="form-control" id="inputPassword" name="inputPassword" maxlength="12"
						required>
				</div>
				<button type="submit" class="btn btn-primary w-100 my-3">Login</button>
				<div class="invalid-feedback my-3 d-block text-start fs-6"><?php echo $errMsg; ?></div>
				<p class="lead fs-5">Don't have an account? <a href="signup.php"
						class="link-primary link-offset-2 link-underline-opacity-25 link-underline-opacity-100-hover">Sign
						up</a></p>
			</form>
		</div>
	</div>
</body>

</html>