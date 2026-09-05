<?php
header('Content-Type: application/json');
header('Access-Control-Allow-Origin: *');

$dataDir = __DIR__ . '/../data';
$dataFile = $dataDir . '/latest.json';

if (!is_dir($dataDir)) {
    mkdir($dataDir, 0775, true);
}

$json = file_get_contents('php://input');
$data = json_decode($json, true);

if (!$data || !isset($data['roomTemp']) || !isset($data['humidity'])) {
    http_response_code(400);
    echo json_encode(['success' => false, 'error' => 'roomTemp and humidity are required']);
    exit;
}

$roomTemp = floatval($data['roomTemp']);
$humidity = floatval($data['humidity']);
// Optional so a dashboard with just the DHT11 wired up (no MQ135 yet) still works.
$airQuality = isset($data['airQuality']) ? intval($data['airQuality']) : 0;

$reading = [
    'roomTemp' => round($roomTemp, 1),
    'humidity' => round($humidity, 1),
    'airQuality' => $airQuality,
    'timestamp' => date('c')
];

if (file_put_contents($dataFile, json_encode($reading), LOCK_EX) === false) {
    http_response_code(500);
    echo json_encode(['success' => false, 'error' => 'Could not write reading']);
    exit;
}

echo json_encode(['success' => true]);
