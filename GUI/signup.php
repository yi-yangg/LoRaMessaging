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
$errID = "";
$errPassword = "";
$errConfirmPassword = "";
$errName = "";
$errMsg = "";
$successMsg = "";

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

	$check = true; // Store validate result

	// Get input value from the form
	$id = sanitise($_POST["inputID"]);
	$password = sanitise($_POST["inputPassword"]);
	$confirmPassword = sanitise($_POST["inputConfirmPassword"]);
	$name = sanitise($_POST["inputName"]);

	// Validate id
	if (!preg_match("/^[a-zA-Z0-9]{4,12}$/", $id)) {
		$errID .= "<p>ID does not meet the requirements</p>";
		$check = false;
	}

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

	// Validate name
	if (!preg_match("/^[a-zA-Z]{1,20}$/", $name)) {
		$errName .= "<p>Name does not meet the requirements</p>";
		$check = false;
	}

	// If validate successfully
	if ($check) {
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

			$query = "SELECT * FROM user WHERE id = '$id';";

			$result = mysqli_query($conn, $query);

			// Check if ID exists, if not, insert a new user record into DB
			if (mysqli_num_rows($result) == 0) {
				$password = hash("sha256", $password); // Hash password

				$query = "INSERT INTO user(id, password, name) 
						VALUES('$id', '$password', '$name');";

				$result = mysqli_query($conn, $query);

				if (!$result) {
					$errMsg .= "<p>Sign-up Failure</p>";
				} else {
					$successMsg .= "<p>Sign-up Success</p>";
					header("location:index.php"); // Redirect to index.php after successful sign-up 
					exit("Redirect");
				}
			} else {
				$errMsg .= "<p>ID has already existed</p>";
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
	<title>Sign up | LoRa Project</title>
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
		<p class="lead fs-2">Sign up</p>

		<div class="pt-5 w-25">
			<!-- User input form -->
			<form method="post" action="signup.php">
				<div class="mb-3 text-start">
					<label for="inputID" class="form-label">ID</label>
					<input type="text" class="form-control <?php if ($errID != "")
						echo "is-invalid"; ?>" id="inputID"
						name="inputID" maxlength="12" required>
					<div class="form-text">Your ID must be 4-12 characters long, contain only letters or numbers.</div>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errID; ?></div>
				</div>
				<div class="mb-3 text-start">
					<label for="inputPassword" class="form-label">Password</label>
					<input type="password"
						class="form-control <?php if ($errPassword != "" || $errConfirmPassword != "")
							echo "is-invalid"; ?>"
						id="inputPassword" name="inputPassword" maxlength="12" required>
					<div class="form-text">Your password must be 4-12 characters long, contain only letters or numbers.
					</div>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errPassword; ?></div>
				</div>
				<div class="mb-3 text-start">
					<label for="inputConfirmPassword" class="form-label">Confirm Password</label>
					<input type="password"
						class="form-control <?php if ($errConfirmPassword != "")
							echo "is-invalid"; ?>"
						id="inputConfirmPassword" name="inputConfirmPassword" maxlength="12" required>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errConfirmPassword; ?></div>
				</div>
				<div class="mb-3 text-start">
					<label for="inputName" class="form-label">Name</label>
					<input type="text" class="form-control <?php if ($errName != "")
						echo "is-invalid"; ?>"
						id="inputName" name="inputName" maxlength="20" required>
					<div class="form-text">Your name must be maximum 20 characters long, contain only letters.</div>
					<div class="invalid-feedback my-3 d-block text-start"><?php echo $errName; ?></div>
				</div>
				<button type="submit" class="btn btn-primary w-100 my-3">Sign up</button>
				<div class="valid-feedback my-3 d-block text-start fs-6"><?php echo $successMsg; ?></div>
				<div class="invalid-feedback my-3 d-block text-start fs-6"><?php echo $errMsg; ?></div>
				<p class="lead fs-5">Already have an account? <a href="index.php"
						class="link-primary link-offset-2 link-underline-opacity-25 link-underline-opacity-100-hover">Login</a>
				</p>
			</form>
		</div>
	</div>
</body>

</html>