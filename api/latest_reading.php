<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

$dataFile = __DIR__ . '/../data/latest.json';

if (!file_exists($dataFile)) {
    echo json_encode(['error' => 'no reading yet']);
    exit;
}

$reading = json_decode(file_get_contents($dataFile), true);

if (!$reading) {
    echo json_encode(['error' => 'no reading yet']);
    exit;
}

// Treat the feed as offline if nothing has arrived in a while (device off / WiFi down)
$ageSeconds = time() - strtotime($reading['timestamp']);
if ($ageSeconds > 15) {
    echo json_encode(['error' => 'stale', 'age' => $ageSeconds]);
    exit;
}

echo json_encode(['success' => true, 'reading' => $reading]);
