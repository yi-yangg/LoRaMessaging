<header>
    <nav class="navbar navbar-expand-lg bg-body-tertiary px-2">
        <div class="container-fluid">
            <a class="navbar-brand" href="index.php">LoRa</a>
            <button class="navbar-toggler" type="button" data-bs-toggle="collapse" data-bs-target="#navbarSupportedContent" aria-controls="navbarSupportedContent" aria-expanded="false" aria-label="Toggle navigation">
            <span class="navbar-toggler-icon"></span>
            </button>
            <div class="collapse navbar-collapse" id="navbarSupportedContent">
                <ul class="navbar-nav me-auto">
                    <li class="nav-item dropdown">
                        <a class="nav-link dropdown-toggle" href="#" role="button" data-bs-toggle="dropdown" aria-expanded="false">
                            My Account
                        </a>
                        <ul class="dropdown-menu">
                            <p class="dropdown-header">Hi, <?php echo $_SESSION["name"]; ?></p>
                            <li><hr class="dropdown-divider"></li>
                            <li><a class="dropdown-item" href="update.php">Update password</a></li>
                            <li><a class="dropdown-item" href="logout.php">Logout</a></li>
                        </ul>
                    </li>
                </ul>
                <ul class="navbar-nav mb-2 mb-lg-0">
                    <li class="nav-item pe-3">
                        <a class="nav-link active" aria-current="page" href="send.php">Send</a>
                    </li>
                    <li class="nav-item pe-3">
                        <a class="nav-link" href="receive.php">Receive</a>
                    </li>
                </ul>
            </div>
        </div>
    </nav>
</header>