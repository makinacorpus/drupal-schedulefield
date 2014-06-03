<?php

/**
 * @file
 * Implement a schedule field.
 */

/**
 * Implements hook_field_info().
 */
function schedulefield_field_info() {
  return array(
    'schedule' => array(
      'label' => t('Schedule'),
      'description' => t('This field stores relative data about scheduling.'),
      'settings' => array(),
      'instance_settings' => array(),
      'default_widget' => 'schedule',
      'default_formatter' => 'schedule',
    ),
  );
}

/**
 * Implements hook_field_is_empty();
 */
function schedulefield_field_is_empty($item, $field) {
  $is_empty = FALSE;

  if (empty($item['date_start']) || empty($item['date_end'])) {
    $is_empty = TRUE;
  }
  else {
    foreach ($item['days'] as $day_week => $periods) {
      foreach ($periods as $period => $times) {
        if ((!empty($times['time_start']) && empty($times['time_end']))
          || (empty($times['time_start']) && !empty($times['time_end']))) {

          $is_empty = TRUE;
          break;
        }
      }
    }
  }

  return $is_empty;
}

/**
 * Implements hook_field_presave().
 */
function schedulefield_field_presave($entity_type, $entity, $field, $instance, $langcode, &$items) {
  global $language;
  $date_format = system_date_format_locale($language->language, $instance['widget']['settings']['date_format']);
  $time_format = system_date_format_locale($language->language, $instance['widget']['settings']['time_format']);

  foreach ($items as &$item) {

    $pid = 0;
    // Schedule update ?
    if (!empty($item['pid'])) {
      // Redondant check if submitted through a form but not if submitted
      // programmatically. No required field means no update necessary.
      if (!schedulefield_field_is_empty($item, $field)) {

        schedulefield_convert_php_to_sql($item, $date_format, $time_format);
        schedulefield_schedule_update($item);
        $pid = $item['pid'];
      }
    }
    else {
      schedulefield_convert_php_to_sql($item, $date_format, $time_format);
      $pid = schedulefield_schedule_insert($item);
    }

    $item = array('pid' => $pid);
  }
}

/**
 * Implements hook_field_delete().
 */
function schedulefield_field_delete($entity_type, $entity, $field, $instance, $langcode, $items) {

  $pids = array();
  foreach ($items as $item) {
    $pids[$item['pid']] = $item['pid'];
  }
  schedulefield_schedule_delete_multiple($pids);
}

/**
 * Implements hook_field_widget_info().
 */
function schedulefield_field_widget_info() {
  return array(
    'schedule' => array(
      'label' => t('Schedule'),
      'field types' => array('schedule'),
      'settings' => array(
        'date_format' => SCHEDULEFIELD_DATE_FORMAT,
        'time_format' => SCHEDULEFIELD_TIME_FORMAT,
        'datepicker_format' => array(),
      ),
    ),
  );
}

/**
 * Implements hook_field_widget_settings_form().
 */
function schedulefield_field_widget_settings_form($field, $instance) {
  $settings = $instance['widget']['settings'];

  $formats = system_get_date_types();
  $options = array();
  foreach ($formats as $format) {
    $options[$format['type']] = $format['title'];
  }

  $form['date_format'] = array(
    '#type' => 'select',
    '#title' => t('Input date format'),
    '#options' => $options,
    '#default_value' => $settings['date_format'],
  );

  $form['time_format'] = array(
    '#type' => 'select',
    '#title' => t('Input time format'),
    '#options' => $options,
    '#default_value' => $settings['time_format'],
  );

  $form['datepicker_format'] = array(
    '#type' => 'fieldset',
    '#title' => t('Datepicker format'),
    '#collapsible' => TRUE,
  );

  $languages = language_list();
  foreach ($languages as $langcode => $language) {
    $form['datepicker_format'][$langcode] = array(
      '#type' => 'textfield',
      '#title' => t('For language %language', array('%language' => $language->name)),
      '#default_value' => !empty($settings['datepicker_format'][$langcode])
        ? $settings['datepicker_format'][$langcode]
        : variable_get('schedulefield_default_date_format_js', 'yy mm dd'),
      '#required' => TRUE,
    );
  }

  return $form;
}

/**
 * Implements hook_field_widget_form().
 */
function schedulefield_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  global $language;

  $settings = $instance['widget']['settings'];
  $date_format = system_date_format_locale($language->language, $settings['date_format']);
  $time_format = system_date_format_locale($language->language, $settings['time_format']);
  $datepicker_format = !empty($settings['datepicker_format'][$language->language])
    ? $settings['datepicker_format'][$language->language]
    : variable_get('schedulefield_default_date_format_js', 'yy mm dd');

  // Store result for validate handler.
  $form_state['schedulefield']['formats'] = array(
    'date' => $date_format,
    'time' => $time_format,
  );

  $element += array(
    '#type' => 'fieldset',
    '#title' => t('Schedule'),
    '#collapsible' => FALSE,
    '#tree' => TRUE,
    '#element_validate' => array('schedulefield_validate_dates'),
    '#attached' => array(
      'css' => array(drupal_get_path('module', 'schedulefield') . '/schedulefield.css'),
    ),
  );

  if (!empty($items[$delta]['pid'])) {
    $element['pid'] = array(
      '#type' => 'value',
      '#value' => $items[$delta]['pid'],
    );
    $schedule = schedulefield_schedule_load($items[$delta]['pid']);
    if ($schedule) {
      schedulefield_convert_sql_to_php($schedule, $date_format, $time_format);
      $items[$delta] += $schedule;
    }
  }

  $element['date_start'] = array(
    '#type' => 'textfield',
    '#title' => t('Date start'),
    '#description' => t('Date format expected : !format', array('!format' => date($date_format))),
    '#default_value' => !empty($items[$delta]['date_start']) ? $items[$delta]['date_start'] : '',
    '#element_validate' => array('schedulefield_validate_date'),
    '#attributes' => array('class' => array('schedulefield-datepicker')),
    '#attached' => array(
      'library' => array(
        array('system', 'ui.datepicker'),
      ),
      'js' => array(
        drupal_get_path('module', 'schedulefield') . '/schedulefield.js',
        array(
          'data' => array(
            'schedulefield' => array(
              'date_format' => $datepicker_format,
            ),
          ),
          'type' => 'setting',
        ),
      ),
    ),
  );

  $element['date_end'] = array(
    '#type' => 'textfield',
    '#title' => t('Date end'),
    '#description' => t('Date format expected : !format', array('!format' => date($date_format))),
    '#default_value' => !empty($items[$delta]['date_end']) ? $items[$delta]['date_end'] : '',
    '#element_validate' => array('schedulefield_validate_date'),
    '#attributes' => array('class' => array('schedulefield-datepicker')),
    '#attached' => array(
      'library' => array(
        array('system', 'ui.datepicker'),
      ),
      'js' => array(
        drupal_get_path('module', 'schedulefield') . '/schedulefield.js',
        array(
          'data' => array(
            'schedulefield' => array(
              'date_format' => $datepicker_format,
            ),
          ),
          'type' => 'setting',
        ),
      ),
    ),
  );

  $element['text_opening'] = array(
    '#type' => 'textfield',
    '#title' => t('Opening text'),
    '#default_value' => isset($items[$delta]['text_opening']) ? $items[$delta]['text_opening'] : '',
    '#maxlength' => 255,
  );

  $element['text_closing'] = array(
    '#type' => 'textfield',
    '#title' => t('Closing text'),
    '#default_value' => isset($items[$delta]['text_closing']) ? $items[$delta]['text_closing'] : '',
    '#maxlength' => 255,
  );

  $periods = schedulefield_get_periods();
  $first_day = variable_get('date_first_day', 0);
  $days_week = range(0, 6);
  if ($first_day !== 0) {
    $previous_days = array_splice($days_week, 0, $first_day);
    $days_week = array_merge($days_week, $previous_days);
  }

  $element['days'] = array(
    '#type' => 'item',
    '#description' => t('Time format expected : !format', array('!format' => date($time_format))),
  );

  foreach ($days_week as $day_week) {

    $element['days'][$day_week] = array(
      '#type' => 'container',
      '#attributes' => array(
        'class' => array('schedulefield-day-line'),
      ),
      'markup' => array(
        '#type' => 'item',
        '#title' => schedulefield_get_dayname($day_week),
      ),
    );

    foreach ($periods as $period => $period_name) {

      $element['days'][$day_week][$period] = array(
        '#type' => 'container',
        'markup' => array(
          '#type' => 'item',
          '#title ' => $period_name,
        ),
      );

      if (!empty($items[$delta]['days'][$day_week][$period]['did'])) {
        $element['days'][$day_week][$period]['did'] = array(
          '#type' => 'value',
          '#value' => $items[$delta]['days'][$day_week][$period]['did'],
        );
      }

      $element['days'][$day_week][$period]['time_start'] = array(
        '#type' => 'textfield',
        '#title' => t('Time start'),
        '#title_display' => 'invisible',
        '#field_prefix' => t('From'),
        '#default_value' => !empty($items[$delta]['days'][$day_week][$period]['time_start']) ? $items[$delta]['days'][$day_week][$period]['time_start'] : '',
        '#element_validate' => array('schedulefield_validate_time'),
        '#size' => 8,
      );
      $element['days'][$day_week][$period]['time_end'] = array(
        '#type' => 'textfield',
        '#title' => t('Time end'),
        '#title_display' => 'invisible',
        '#field_prefix' => t('To'),
        '#default_value' => !empty($items[$delta]['days'][$day_week][$period]['time_end']) ? $items[$delta]['days'][$day_week][$period]['time_end'] : '',
        '#element_validate' => array('schedulefield_validate_time'),
        '#size' => 8,
      );

      $element['days'][$day_week][$period]['#element_validate'][] = 'schedulefield_validate_times';
    }
  }

  return $element;
}

/**
 * Date element validate.
 */
function schedulefield_validate_date($element, &$form_state, $form) {
  if (!empty($element['#value'])) {

    $format = $form_state['schedulefield']['formats']['date'];
    $datetime = DateTime::createFromFormat($format, $element['#value']);
    if (!$datetime) {
      form_error($element, t('Field %name : the date is invalid.', array('%name' => $element['#title'])));
    }
  }
}

/**
 * Date elements validate.
 */
function schedulefield_validate_dates($element, &$form_state, $form) {

  // Check dates.
  if (!form_get_error($element['date_start']) && !form_get_error($element['date_end'])
    && (!empty($element['date_start']['#value']) || !empty($element['date_end']['#value']))) {

    if (empty($element['date_start']['#value']) || empty($element['date_end']['#value'])) {
      form_error($element, t('Both fields %start and %end are mutually required.', array('%start' => $element['date_start']['#title'], '%end' => $element['date_end']['#title'])));
    }
    else {

      // Time generated would be the approximatively the same and start is deals
      // first so it doesn't really matter.
      $format = $form_state['schedulefield']['formats']['date'];
      $date_start = DateTime::createFromFormat($format, $element['date_start']['#value'])->getTimestamp();
      $date_end = DateTime::createFromFormat($format, $element['date_end']['#value'])->getTimestamp();

      if ($date_start > $date_end) {
        form_error($element, t('Field %start could not be greater than field %end.', array('%start' => $element['date_start']['#title'], '%end' => $element['date_end']['#title'])));
      }
    }
  }
}

/**
 * Time element validate.
 */
function schedulefield_validate_time($element, &$form_state, $form) {
  if (!empty($element['#value'])) {

    $format = $form_state['schedulefield']['formats']['time'];
    $datetime = DateTime::createFromFormat($format, $element['#value']);
    if (!$datetime) {
      form_error($element, t('The time is invalid.'));
    }
  }
}

/**
 * Time elements validate.
 */
function schedulefield_validate_times($element, &$form_state, $form) {

  if (!form_get_error($element['time_start']) && !form_get_error($element['time_end'])
    && (!empty($element['time_start']['#value']) || !empty($element['time_end']['#value']))) {

    if (empty($element['time_start']['#value']) || empty($element['time_end']['#value'])) {
      form_error($element, t('Both fields %start and %end are mutually required.', array('%start' => $element['time_start']['#title'], '%end' => $element['time_end']['#title'])));
    }
    else {

      // Days generated would be the same and start is deals first so it doesn't
      // really matter
      $format = $form_state['schedulefield']['formats']['time'];
      $time_start = DateTime::createFromFormat($format, $element['time_start']['#value'])->getTimestamp();
      $time_end = DateTime::createFromFormat($format, $element['time_end']['#value'])->getTimestamp();

      if ($time_start > $time_end) {
        form_error($element, t('Field %start could not be greater than field %end.', array('%start' => $element['time_start']['#title'], '%end' => $element['time_end']['#title'])));
      }
    }
  }
}

/**
 * Implements hook_field_formatter_info().
 */
function schedulefield_field_formatter_info() {
  $formatters = array(
    'schedule' => array(
      'label' => t('Schedule'),
      'field types' => array('schedule'),
      'settings' => array(
        'date_format' => SCHEDULEFIELD_DATE_FORMAT,
        'time_format' => SCHEDULEFIELD_TIME_FORMAT,
      ),
    ),
  );

  return $formatters;
}

/**
 * Implements hook_field_formatter_settings_form();
 */
function schedulefield_field_formatter_settings_form($field, $instance, $view_mode, $form, $form_state) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $formats = system_get_date_types();
  $options = array();
  foreach ($formats as $format) {
    $options[$format['type']] = $format['title'];
  }

  $element['date_format'] = array(
    '#type' => 'select',
    '#title' => t('Date format'),
    '#options' => $options,
    '#default_value' => $settings['date_format'],
  );

  $element['time_format'] = array(
    '#type' => 'select',
    '#title' => t('Time format'),
    '#options' => $options,
    '#default_value' => $settings['time_format'],
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary();
 */
function schedulefield_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'];

  $summary = array();

  $types = system_get_date_types();
  $summary[] = t('Date format : %format', array('%format' => $types[$settings['date_format']]['title']));
  $summary[] = t('Time format : %format', array('%format' => $types[$settings['time_format']]['title']));

  return implode('<br />', $summary);
}

/**
 * Implements hook_field_prepare_view().
 */
function schedulefield_field_prepare_view($entity_type, $entities, $field, $instances, $langcode, &$items) {

  // Preload all schedules.
  $pids = array();
  foreach ($items as $entity_id => $item) {
    foreach ($item as $delta => $values) {
      $pids[$values['pid']] = $values['pid'];
    }
  }
  $schedules = schedulefield_schedule_load_multiple($pids);

  foreach ($items as &$item) {
    foreach ($item as $delta => &$values) {
      $values += $schedules[$values['pid']];
    }
  }
}

/**
 * Implements hook_field_formatter_view().
 */
function schedulefield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  global $language;
  $element = array();

  $first_day = variable_get('date_first_day', 0);
  $date_format = system_date_format_locale($language->language, $display['settings']['date_format']);
  $time_format = system_date_format_locale($language->language, $display['settings']['time_format']);

  foreach ($items as $delta => $item) {

    // Convert SQL format to drupal formats.
    schedulefield_convert_sql_to_php($item, $date_format, $time_format);

    // Re-order days.
    if (!empty($items['days'])) {
      ksort($items['days']);
      if ($first_day > 0) {
        foreach ($items['days'] as $day_week => $values) {
          if ($day_week >= $first_day) {
            break;
          }
          $items['days'][$day_week] = $values;
        }
      }
    }

    $element[$delta] = array(
      '#theme' => 'schedulefield_formatter',
      '#item' => $item,
    );
  }

  return $element;
}

/**
 * Returns HTML for a default schedule field.
 *
 * @ingroup themeable
 */
function theme_schedulefield_formatter($variables) {
  $item = $variables['item'];
  $output = '';

  $output .= '<div class="schedulefield">';
  $output .= '<p>' . t('From !start to !end', array('!start' => $item['date_start'], '!end' => $item['date_end']));

  if (!empty($item['text_opening'])) {
    $output .= '<br />' . check_plain($item['text_opening']);
  }
  if (!empty($item['text_closing'])) {
    $output .= '<br />' . check_plain($item['text_closing']);
  }

  foreach ($item['days'] as $day_week => $periods) {
    $output .= '<br /><br />' . schedulefield_get_dayname($day_week) . ' : ';
    $days = array();
    foreach ($periods as $period => $datetimes) {
      $days[] = t('!period : From !time_start to !time_end', array('!period' => ucfirst($period), '!time_start' => $datetimes['time_start'], '!time_end' => $datetimes['time_end']));
    }
    $output .= implode(', ', $days);
  }

  $output .= '</p></div>';
  return $output;
}