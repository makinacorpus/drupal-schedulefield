<?php

/**
 * Function helper to load schedules.
 *
 * @param $pids
 *   An array of pids to load. FALSE to load all.
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
function schedulefield_schedule_load_multiple($pids, $reset = FALSE) {
  $cache =& drupal_static(__FUNCTION__, array());

  if ($reset) {
    $cache = array();
  }

  // Extract requested ID's from cache if any.
  $passed_pids = FALSE === $pids || $pids ? $pids : NULL;
  $pids = is_array($passed_pids) ? array_diff($passed_pids, array_keys($cache)) : $passed_pids;
  if (FALSE === $pids || $pids) {

    // Get all/remaining schedules from DB.
    $query = db_select('schedulefield_period', 'sp')
      ->fields('sp');

    if ($pids) {
      $query->condition('sp.pid', $pids);
    }

    $query->leftJoin('schedulefield_day', 'sd', 'sp.pid = sd.pid');
    $query->fields('sd', array('did', 'day_week', 'period', 'time_start', 'time_end'));
    $result = $query->execute()->fetchAll();

    foreach ($result as $data) {

      $cache[$data->pid]['date_start'] = $data->date_start;
      $cache[$data->pid]['date_end'] = $data->date_start;
      $cache[$data->pid]['text_opening'] = $data->text_opening;
      $cache[$data->pid]['text_closing'] = $data->text_closing;

      $cache[$data->pid]['days'] = array();
      if (isset($data->did)) {
        $cache[$data->pid]['days'][$data->day_week][$data->period]['did'] = $data->did;
        $cache[$data->pid]['days'][$data->day_week][$data->period]['time_start'] = $data->time_start;
        $cache[$data->pid]['days'][$data->day_week][$data->period]['time_end'] = $data->time_end;
      }
    }
  }

  return $passed_pids ? array_intersect_key($cache, array_flip($passed_pids)) : $cache;
}

/**
 * Function helper to load a schedule.
 *
 * @param $pid
 *   A schedule ID to load.
 * @param $reset
 *   Whether to reset internal cache.
 */
function schedulefield_schedule_load($pid, $reset = FALSE) {
  $schedules = schedulefield_schedule_load_multiple(array($pid), $reset);
  return $schedules ? reset($schedules) : array();
}

/**
 * Function helper to insert a new schedule.
 *
 * @param $data
 *   An array of data schedule.
 *
 * @return
 *   The new schedule ID created. FALSE otherwise.
 */
function schedulefield_schedule_insert(array $data) {
  $pid = FALSE;

  $trx = db_transaction();
  try {

    $pid = db_insert('schedulefield_period')
      ->fields(array(
        'date_start' => $data['date_start'],
        'date_end' => $data['date_end'],
        'text_opening' => $data['text_opening'],
        'text_closing' => $data['text_closing'],
      ))
      ->execute();

    if ($pid) {

      foreach ($data['days'] as $day_week => $periods) {
        foreach ($periods as $period => $times) {
          if (!empty($times['time_start']) && !empty($times['time_end'])) {

            db_insert('schedulefield_day')
              ->fields(array(
                'pid' => $pid,
                'day_week' => $day_week,
                'period' => $period,
                'time_start' => $times['time_start'],
                'time_end' => $times['time_end'],
              ))
              ->execute();
          }
        }
      }
    }
  }
  catch (Exception $e) {
    $trx->rollback();
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

  $trx = db_transaction();
  try {

    db_update('schedulefield_period')
      ->fields(array(
        'date_start' => $data['date_start'],
        'date_end' => $data['date_end'],
        'text_opening' => $data['text_opening'],
        'text_closing' => $data['text_closing'],
      ))
      ->condition('pid', $data['pid'])
      ->execute();

    // In case we are adding new days and removing old, this is much easy to
    // do it in that way. However, we may have some SQL transaction lock
    // troubles if default MySQL transaction level is keep.
    db_delete('schedulefield_day')
      ->condition('pid', $data['pid'])
      ->execute();

    foreach ($data['days'] as $day_week => $periods) {
      foreach ($periods as $period => $times) {
        if (!empty($times['time_start']) && !empty($times['time_end'])) {

          db_insert('schedulefield_day')
            ->fields(array(
              'pid' => $data['pid'],
              'day_week' => $day_week,
              'period' => $period,
              'time_start' => $times['time_start'],
              'time_end' => $times['time_end'],
            ))
            ->execute();
        }
      }
    }
  }
  catch (Exception $e) {
    $trx->rollback();
    watchdog_exception('schedulefield', $e);
    throw $e;
  }
}

/**
 * Function helper to delete multiple schedules.
 */
function schedulefield_schedule_delete_multiple(array $pids) {

  if ($pids) {

    $trx = db_transaction();
    try {

      db_delete('schedulefield_day')
        ->condition('pid', $pids)
        ->execute();
      db_delete('schedulefield_period')
        ->condition('pid', $pids)
        ->execute();
    }
    catch (Exception $e) {
      $trx->rollback();
      watchdog_exception('schedulefield', $e);
      throw $e;
    }
  }
}
