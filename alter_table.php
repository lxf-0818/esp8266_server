<?php

declare(strict_types=1);

/*
 * Simple ALTER TABLE runner.
 *
 * Usage:
 *   php alter_table.php
 *
 * Optional environment variables:
 *   DB_HOST, DB_PORT, DB_NAME, DB_USER, DB_PASS
 */

$host = getenv('DB_HOST') ?: '127.0.0.1';
$port = (int) (getenv('DB_PORT') ?: '3306');
$db   = getenv('DB_NAME') ?: 'esp';
$user = getenv('DB_USER') ?: 'root';
$pass = getenv('DB_PASS') ?: '';

$mysqli = new mysqli($host, $user, $pass, $db, $port);
if ($mysqli->connect_errno) {
    fwrite(STDERR, "DB connection failed: {$mysqli->connect_error}\n");
    exit(1);
}

/*
 * Edit this array with the ALTER statements you need.
 */
$alterStatements = [
    "ALTER TABLE sensor_data ADD COLUMN location VARCHAR(64) NULL AFTER sensor",
    "ALTER TABLE sensor_data ADD COLUMN updated_at TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP",
];

foreach ($alterStatements as $sql) {
    echo "Running: {$sql}\n";
    if (!$mysqli->query($sql)) {
        fwrite(STDERR, "Error: {$mysqli->error}\n");
        $mysqli->close();
        exit(1);
    }
    echo "OK\n";
}

$mysqli->close();
echo "Schema update complete.\n";
