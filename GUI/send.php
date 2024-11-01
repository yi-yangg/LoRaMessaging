<?php
session_start();

// Redirect users to send.php if not login
if (!isset($_SESSION["id"])) {
  header("location:index.php");
  exit("Redirect");
}
?>
<!DOCTYPE html>
<html lang="en">

<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Send | LoRa Project</title>
  <!-- Include Bootstrap framework -->
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css" rel="stylesheet"
    integrity="sha384-QWTKZyjpPEjISv5WaRU9OFeRpok6YctnYmDr5pNlyT2bRjXh0JMhjY6hW+ALEwIH" crossorigin="anonymous">
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js"
    integrity="sha384-YvpcrYf0tY3lHB60NNkmXc5s9fDVZLESaAA55NDzOxhy9GkcIdslK1eN7N6jIeHz"
    crossorigin="anonymous"></script>
  <!-- Include Web Serial API js functions -->
  <script src="scripts/common.js"></script>
  <script src="scripts/send.js"></script>
</head>

<body>
  <!-- Include header -->
  <?php
  include_once("headerAfterLogin.inc");
  ?>

  <!-- Port connect buttons -->
  <div class="container-fluid text-center pt-5 d-flex flex-column align-items-center">
    <!-- Heading -->
    <h1 class="display-3">LoRa Messaging System</h1>
    <p class="lead fs-2">Message Sending Panel</p>
    <div class="pt-5 d-flex flex-column w-50 align-items-center">
      <p class="mb-2 lead fs-4">Transmitter COM Port: <span id="port-info" class="mt-2 text-danger">No port
          selected</span></p>
      <div>
        <button type="button" id="select-button" class="btn btn-primary mb-2">Select Transmitter Port</button>
        <button type="button" id="disconnect-button" class="btn btn-danger mb-2 d-none">Disconnect Transmitter
          Port</button>
      </div>
    </div>
  </div>

  <!-- Hidden before connect -->
  <div class="container-fluid text-center py-5 d-flex flex-column align-items-center d-none mx-auto"
    id="monitor-container">
    <!-- Message input -->
    <div class="d-flex pt-5 w-50">
      <input type="hidden" id="password-info" value="<?php echo hash("md5", $_SESSION["password"]); ?>">
      <input type="text" id="message-input" class="form-control me-4" placeholder="Enter a message">
      <button type="button" id="send-button" class="btn btn-dark ms-2">Send</button>
    </div>

    <!-- Serial monitor -->
    <div class="pt-5 w-50 text-start">
      <h3>Serial Monitor
        <button type="button" id="clear-button" class="btn btn-dark ms-4">Clear</button>
      </h3>
      <textarea id="serial-output" class="form-control" style="resize:none" rows="10" readonly disabled
        placeholder="Received messages"></textarea>
    </div>
  </div>

  <!-- Toast -->
  <div class="toast align-items-center position-absolute top-0 start-50 translate-middle-x" role="alert"
    aria-live="assertive" aria-atomic="true" id="liveToast">
    <div class="d-flex">
      <div class="toast-body text-danger" id="toast-text">
      </div>
      <button type="button" class="btn-close me-2 m-auto" data-bs-dismiss="toast" aria-label="Close"></button>
    </div>
  </div>
</body>

</html>