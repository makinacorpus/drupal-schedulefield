<?php

/**
 * Implements hook_migrate_api().
 */
function schedulefield_migrate_api() {
  return [
    'api' => 2,
    'field handlers' => [
      'MakinaCorpus\Schedule\Migrate\ScheduleFieldHandler',
    ],
  ];
}
