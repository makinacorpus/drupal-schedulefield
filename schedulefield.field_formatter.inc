<?php

/**
 * Implements hook_field_formatter_info().
 */
function schedulefield_field_formatter_info() {
  return [
    'schedule' => [
      'label'       => t('Schedule'),
      'field types' => ['schedule'],
    ],
  ];
}

/**
 * Implements hook_field_formatter_settings_form();
 */
function schedulefield_field_formatter_settings_form($field, $instance, $view_mode, $form, $form_state) {
  $element = [];

  $display = $instance['display'][$view_mode];
  $settings = $display['settings'] + schedulefield_field_formatter_settings_default();

  $formats = system_get_date_types();
  $options = array();
  foreach ($formats as $format) {
    $options[$format['type']] = $format['title'];
  }

  $element['date_format'] = array(
    '#type'           => 'select',
    '#title'          => t('Date format'),
    '#options'        => $options,
    '#default_value'  => $settings['date_format'],
  );

  $element['time_format'] = array(
    '#type'           => 'select',
    '#title'          => t('Time format'),
    '#options'        => $options,
    '#default_value'  => $settings['time_format'],
  );

  $element['md_enabled'] = array(
    '#title'          => t('Automatically add microdata'),
    '#type'           => 'checkbox',
    '#default_value'  => $settings['md_enabled'],
  );

  $element['md_property'] = array(
    '#title'          => t('Microdata property name'),
    '#type'           => 'textfield',
    '#description'    => t('Fill in this field if you want to define the hours as a property of another microdata type. Microdata must be enabled.'),
    '#default_value'  => $settings['md_property'],
  );

  return $element;
}

/**
 * Implements hook_field_formatter_settings_summary();
 */
function schedulefield_field_formatter_settings_summary($field, $instance, $view_mode) {
  $display = $instance['display'][$view_mode];
  $settings = $display['settings'] + schedulefield_field_formatter_settings_default();

  $summary = [];
  $types = system_get_date_types();

  if ($settings['date_format'] == 'custom') {
    $summary[] = t('Date format: %format', ['%format' => t('by default (dd/mm/yyyy)')]);
  } else {
    $summary[] = t('Date format: %format', ['%format' => $types[$settings['date_format']]['title']]);
  }

  if ($settings['time_format'] == 'custom') {
    $summary[] = t('Time format: %format', ['%format' => t('by default (hh:mm)')]);
  } else {
    $summary[] = t('Time format: %format', array('%format' => $types[$settings['time_format']]['title']));
  }

  if (isset($settings['md_enabled']) && $settings['md_enabled']) {
    $summary[] = t('Microdata enabled');
    if (!empty($settings['md_property'])) {
      $summary[] = t('Microdata property name: @name', ['@name' => $settings['md_property']]);
    }
  } else {
    $summary[] = t('Microdata disabled');
  }

  return implode('<br/>', $summary);
}

/**
 * Implements hook_field_prepare_view().
 */
function schedulefield_field_prepare_view($entity_type, $entities, $field, $instances, $langcode, &$items) {
  // Preload all periods.
  $pids = array();

  foreach ($items as $item) {
    foreach ($item as $delta => $value) {
      if (!empty($value['pid'])) {
        $pids[$value['pid']] = $value['pid'];
      }
    }
  }

  $periods = schedulefield_schedule_load_multiple($pids);

  if (!empty($periods)) {
    foreach ($items as &$item) {
      foreach ($item as $delta => &$value) {
        if (!isset($periods[$value['pid']])) {
          continue;
        }

        $value += $periods[$value['pid']];

        // Ensures slots are well ordered
        foreach ($value['days'] as $day => &$slots) {
          $ordered_slots = [];
          foreach (schedulefield_get_slots() as $key => $label) {
            if (isset($slots[$key])) {
              $ordered_slots[$key] = $slots[$key];
            }
          }
          $slots = $ordered_slots;
        }
      }
    }
  }
}

/**
 * Implements hook_field_formatter_view().
 */
function schedulefield_field_formatter_view($entity_type, $entity, $field, $instance, $langcode, $items, $display) {
  $element = array();
  $settings = $display['settings'] + schedulefield_field_formatter_settings_default();

  foreach ($items as $delta => $item) {
    $fdow = variable_get('date_first_day', 1);

    // Reorders days
    if (!empty($item['days'])) {
      // Ensures days are correctly ordered by ascending day number
      ksort($item['days']);
      // Makes the first day of the week the first of the array
      if ($fdow > 0) {
        foreach ($item['days'] as $day => $hours) {
          if ($day >= $fdow) {
            break;
          }
          unset($item['days'][$day]);
          $item['days'][$day] = $hours;
        }
      }
    }

    // Converts dates and times from DateTime/DateInterval to formatted string
    if (isset($item['date_start'])) {
      if ($settings['md_enabled']) {
        $item['date_start_iso'] = $item['date_start']->format('Y-m-d');
      }
      $item['date_start'] = format_date($item['date_start']->getTimestamp(), $settings['date_format'], $settings['date_format_custom']);
    }

    if (isset($item['date_end'])) {
      if ($settings['md_enabled']) {
        $item['date_end_iso'] = $item['date_end']->format('Y-m-d');
      }
      $item['date_end'] = format_date($item['date_end']->getTimestamp(), $settings['date_format'], $settings['date_format_custom']);
    }

    foreach ($item['days'] as $day => &$hours) {
      foreach ($hours as $slot => &$times) {
        if (isset($times['time_start'])) {
          if ($settings['md_enabled']) {
            $times['time_start_iso'] = $times['time_start']->format('%H:%I');
          }

          $date = (new \DateTime('today'))->add($times['time_start']);
          $times['time_start'] = format_date($date->getTimestamp(), $settings['time_format'], $settings['time_format_custom']);
        }

        if (isset($times['time_end'])) {
          if ($settings['md_enabled']) {
            $times['time_end_iso'] = $times['time_end']->format('%H:%I');
          }

          $date = (new \DateTime('today'))->add($times['time_end']);
          $times['time_end'] = format_date($date->getTimestamp(), $settings['time_format'], $settings['time_format_custom']);
        }
      }
    }

    $element[$delta] = array(
      '#theme' => 'schedulefield_item',
      '#item' => $item,
      '#md_enabled' => $settings['md_enabled'],
      '#md_property' => $settings['md_property'],
    );
  }

  return $element;
}

/**
 * Provides the formatter's default settings
 */
function schedulefield_field_formatter_settings_default() {
  return array(
    'date_format' => 'custom',
    'date_format_custom' => 'd/m/Y',
    'time_format' => 'custom',
    'time_format_custom' => 'H:i',
    'md_enabled' => false,
    'md_property' => '',
  );
}
