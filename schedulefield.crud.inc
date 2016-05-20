<?php

/**
 * Function helper to load schedules.
 *
 * @param $pids
 *   An array of pids to load. false to load all.
 * @param $reset
 *   Whether to reset internal cache.
 *
 * @return
 *   An array keyed by pid and containing an array with keys :
 *   - date_start
 *   - date_end
 *   - text_opening
 *   - text_closing
 *   - days : an array keyed by day of the week and containing an array keyed
 *            by period type and containing an array with :
 *            - time_start
 *            - time_end
 */
function schedulefield_schedule_load_multiple($pids, $reset = false) {
  $cache = &drupal_static(__FUNCTION__, []);

  if ($reset) {
    $cache = [];
  }

  // Extract requested ID's from cache if any.
  $passed_pids = false === $pids || $pids ? $pids : null;
  $pids = is_array($passed_pids) ? array_diff($passed_pids, array_keys($cache)) : $passed_pids;
  if (false === $pids || $pids) {

    // Get all/remaining schedules from DB.
    $query = db_select('schedulefield_period', 'sp')->fields('sp');

    if ($pids) {
      $query->condition('sp.pid', $pids);
    }

    $query->leftJoin('schedulefield_day', 'sd', 'sp.pid = sd.pid');
    $query->fields('sd', ['did', 'day_week', 'period', 'time_start', 'time_end']);
    $result = $query->execute()->fetchAll();

    foreach ($result as $data) {

      if (!isset($cache[$data->pid])) {
        $cache[$data->pid]['date_start'] = isset($data->date_start) ? \DateTime::createFromFormat(UDATE_PHP_DATE, $data->date_start) : null;
        $cache[$data->pid]['date_end'] = isset($data->date_end) ? \DateTime::createFromFormat(UDATE_PHP_DATE, $data->date_end) : null;
        $cache[$data->pid]['text_opening'] = !empty($data->text_opening) ? $data->text_opening : '';
        $cache[$data->pid]['text_closing'] = !empty($data->text_closing) ? $data->text_closing : '';
        $cache[$data->pid]['days'] = [];
      }

      // Have we days for this period?
      if (isset($data->did)) {
        $cache[$data->pid]['days'][$data->day_week][$data->period]['did'] = $data->did;

        if ($data->time_start) {
          $items = explode(':', $data->time_start);
          $start = new \DateInterval('PT' . $items[0] . 'H' . $items[1] . 'M' . $items[2] . 'S');
        } else {
          $start = null;
        }

        if ($data->time_end) {
          $items = explode(':', $data->time_end);
          $end = new \DateInterval('PT' . $items[0] . 'H' . $items[1] . 'M' . $items[2] . 'S');
        } else {
          $end = null;
        }

        $cache[$data->pid]['days'][$data->day_week][$data->period]['time_start'] = $start;
        $cache[$data->pid]['days'][$data->day_week][$data->period]['time_end'] = $end;
      }
    }
  }

  return $passed_pids === false ? $cache : (is_array($passed_pids) ? array_intersect_key($cache, array_flip($passed_pids)) : array());
}

/**
 * Function helper to load a schedule.
 *
 * @param $pid
 *   A schedule ID to load.
 * @param $reset
 *   Whether to reset internal cache.
 */
function schedulefield_schedule_load($pid, $reset = false) {
  $schedules = schedulefield_schedule_load_multiple([$pid], $reset);
  return $schedules ? reset($schedules) : [];
}

/**
 * Function helper to insert a new schedule.
 *
 * @param $data
 *   An array of data schedule.
 *
 * @return
 *   The new schedule ID created. false otherwise.
 */
function schedulefield_schedule_insert(array $data) {
  $pid = false;

  $tx = null;
  try {
    $tx = db_transaction();

    $pid = db_insert('schedulefield_period')
      ->fields([
        'date_start'    => !empty($data['date_start']) ? $data['date_start']->format(UDATE_PHP_DATE) : null,
        'date_end'      => !empty($data['date_end']) ? $data['date_end']->format(UDATE_PHP_DATE) : null,
        'text_opening'  => !empty($data['text_opening']) ? $data['text_opening'] : '',
        'text_closing'  => !empty($data['text_closing']) ? $data['text_closing'] : '',
      ])
      ->execute()
    ;

    if ($pid) {
      foreach ($data['days'] as $day_week => $periods) {
        foreach ($periods as $period => $times) {
          if (!empty($times['time_start']) || !empty($times['time_end'])) {
            db_insert('schedulefield_day')
              ->fields([
                'pid'         => $pid,
                'day_week'    => $day_week,
                'period'      => $period,
                'time_start'  => !empty($times['time_start']) ? $times['time_start']->format(UDATE_TIME_FORMAT_MINUTE) : null,
                'time_end'    => !empty($times['time_end']) ? $times['time_end']->format(UDATE_TIME_FORMAT_MINUTE) : null,
              ])
              ->execute()
            ;
          }
        }
      }
    }

    unset($tx);

  } catch (\Exception $e) {
    if ($tx) {
      try {
        $tx->rollback();
      } catch (\Exception $e2) {}
    }
    watchdog_exception('schedulefield', $e);
    throw $e;
  }

  return $pid;
}

/**
 * Function helper to update a schedule.
 *
 * @param $data
 *   An array of data schedule.
 */
function schedulefield_schedule_update(array $data) {

  $tx = null;
  try {
    $tx = db_transaction();

    db_update('schedulefield_period')
      ->fields([
        'date_start'    => !empty($data['date_start']) ? $data['date_start']->format(UDATE_PHP_DATE) : null,
        'date_end'      => !empty($data['date_end']) ? $data['date_end']->format(UDATE_PHP_DATE) : null,
        'text_opening'  => !empty($data['text_opening']) ? $data['text_opening'] : '',
        'text_closing'  => !empty($data['text_closing']) ? $data['text_closing'] : '',
      ])
      ->condition('pid', $data['pid'])
      ->execute()
    ;

    // In case we are adding new days and removing old, this is much easy to
    // do it in that way. However, we may have some SQL transaction lock
    // troubles if default MySQL transaction level is keep.
    db_delete('schedulefield_day')->condition('pid', $data['pid'])->execute();

    foreach ($data['days'] as $day_week => $periods) {
      foreach ($periods as $period => $times) {
        if (!empty($times['time_start']) && !empty($times['time_end'])) {

          db_insert('schedulefield_day')
            ->fields([
              'pid'         => $data['pid'],
              'day_week'    => $day_week,
              'period'      => $period,
              'time_start'  => !empty($times['time_start']) ? $times['time_start']->format(UDATE_TIME_FORMAT_MINUTE) : null,
              'time_end'    => !empty($times['time_end']) ? $times['time_end']->format(UDATE_TIME_FORMAT_MINUTE) : null,
            ])
            ->execute()
          ;
        }
      }
    }

    unset($tx);

  } catch (\Exception $e) {
    if ($tx) {
      try {
        $tx->rollback();
      } catch (\Exception $e2) {}
    }
    watchdog_exception('schedulefield', $e);
    throw $e;
  }
}

/**
 * Function helper to delete multiple schedules.
 */
function schedulefield_schedule_delete_multiple(array $pids) {
  if ($pids) {
    $tx = null;

    try {
      $tx = db_transaction();

      db_delete('schedulefield_day')->condition('pid', $pids)->execute();
      db_delete('schedulefield_period')->condition('pid', $pids)->execute();

      unset($tx);

    } catch (\Exception $e) {
      if ($tx) {
        try {
          $tx->rollback();
        } catch (\Exception $e2) {}
      }
      watchdog_exception('schedulefield', $e);
      throw $e;
    }
  }
}
