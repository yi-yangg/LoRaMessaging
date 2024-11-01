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
$errPassword = "";
$errConfirmPassword = "";
$errMsg = "";
$successMsg = "";

// Redirect users to send.php if not login
if (!isset($_SESSION["id"])) {
	header("location:index.php");
	exit("Redirect");
}

// Check input data
if (isset($_POST["inputPassword"])) {
	$conn = NULL;

	// Connect to DB
	try {
		$conn = mysqli_connect($host, $user, $pwd, $sql_db);
	} catch (mysqli_sql_exception $e) {
		// handle the error
	}

	$check = true; // Store validate result

	// Get input value from the form
	$password = sanitise($_POST["inputPassword"]);
	$confirmPassword = sanitise($_POST["inputConfirmPassword"]);

	// Validate password
	if (!preg_match("/^[a-zA-Z0-9]{4,12}$/", $password)) {
		$errPassword .= "<p>Password does not meet the requirements</p>";
		$check = false;
	}

	// Confirm password
	if ($password != $confirmPassword) {
		$errConfirmPassword .= "<p>Password does not match</p>";
		$check = false;
	}

	// If validate successfully
	if ($check) {
		// Check DB connection
		if (!$conn) {
			$errMsg .= "<p>Database Connection Failure</p>";
		} else {
			// Password update logic
			$id = $_SESSION["id"];
			$password = hash("sha256", $password); // Hash password
			$date = date("Y-m-d"); // Get current date

			$query = "SELECT * FROM user WHERE id = '$id';";

			$result = mysqli_query($conn, $query);

			$row = mysqli_fetch_assoc($result);

			// Compare new and old passwords, if different, update user password and update date
			if ($row["password"] != $password) {
				$query = "UPDATE user
						SET password = '$password', updated_at = '$date'
						WHERE id = '$id';";

				$result = mysqli_query($conn, $query);

				if (!$result) {
					$errMsg .= "<p>Update Failure</p>";
				} else {
					$_SESSION["password"] = $password;
					$successMsg .= "<p>Update Success</p>";
				}
			} else {
				$errMsg .= "<p>Your new password cannot be the same as your old password</p>";
			}

			mysqli_close($conn);
		}
	}
}
?>
<!DOCTYPE html>
<html lang="en">

<head>
	<meta charset="UTF-8">
	<meta name="viewport" content="width=device-width, initial-scale=1.0">
	<title>Password Update | LoRa Project</title>
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
	include_once("headerAfterLogin.inc");
	?>

	<div class="container-fluid text-center py-5 d-flex flex-column align-items-center mx-auto">
		<!-- Heading -->
		<h1 class="display-3">LoRa Messaging System</h1>
		<p class="lead fs-2">Password Update</p>

		<div class="pt-5 w-25">
			<!-- User input form -->
			<form method="post" action="update.php">
				<div class="mb-3 text-start">
					<label for="inputPassword" class="form-label">Password</label>
					<input type="password" class="form-control <?php if ($errPassword != "" || $errConfirmPassword != "")
						echo "is-invalid"; ?>" id="inputPassword" name="inputPassword" maxlength="12" required>
					<div class="form-text">Your password must be 4-12 characters long, contain only letters or numbers.
					</div>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errPassword; ?></div>
				</div>
				<div class="mb-3 text-start">
					<label for="inputConfirmPassword" class="form-label">Confirm Password</label>
					<input type="password" class="form-control <?php if ($errConfirmPassword != "")
						echo "is-invalid"; ?>" id="inputConfirmPassword" name="inputConfirmPassword" maxlength="12" required>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errConfirmPassword; ?></div>
				</div>
				<button type="submit" class="btn btn-primary w-100 my-3">Update</button>
				<div class="valid-feedback my-3 d-block text-start fs-6"><?php echo $successMsg; ?></div>
				<div class="invalid-feedback my-3 d-block text-start fs-6"><?php echo $errMsg; ?></div>
			</form>
		</div>
	</div>
</body>

</html>