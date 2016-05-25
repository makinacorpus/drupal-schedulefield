<?php if (isset($item['date_start']) || isset($item['date_end'])): ?>
<div class="schedulefield-period">
  <?php
    if (isset($item['date_start']) && isset($item['date_end'])) {
      print t('From !start to !end', array('!start' => $item['date_start'], '!end' => $item['date_end']));
    }
    elseif (isset($item['date_start'])) {
      print t('From !start', array('!start' => $item['date_start']));
    }
    elseif (isset($item['date_end'])) {
      print t('To !end', array('!end' => $item['date_end']));
    }
  ?>
</div>
<?php endif; ?>

<?php if (!empty($item['text_opening'])): ?>
<p class="schedulefield-text-opening">
  <?php print check_plain($item['text_opening']); ?>
</p>
<?php endif; ?>

<?php if (!empty($item['text_closing'])): ?>
<p class="schedulefield-text-closing">
  <?php print check_plain($item['text_closing']); ?>
</p>
<?php endif; ?>

<?php foreach ($item['days'] as $day => $slots): ?>
<div class="schedulefield-day">
  <?php print schedulefield_get_day_name($day); ?>
</div>

<?php foreach ($slots as $slot => $hours): ?>
<div class="schedulefield-slot"<?php if ($md_enabled) print $itemprop . $itemscope; ?>>
  <?php if ($md_enabled): ?>
    <?php if (isset($item['date_start_iso'])): ?>
    <meta itemprop="validFrom" content="<?php print $item['date_start_iso']; ?>"/>
    <?php endif; ?>

    <?php if (isset($item['date_end_iso'])): ?>
    <meta itemprop="validThrough" content="<?php print $item['date_end_iso']; ?>"/>
    <?php endif; ?>

    <link itemprop="dayOfWeek" href="<?php print schedulefield_get_day_microdata_url($day); ?>"/>

    <?php if (isset($hours['time_start_iso'])): ?>
    <meta itemprop="opens" content="<?php print $hours['time_start_iso']; ?>"/>
    <?php endif; ?>

    <?php if (isset($hours['time_end_iso'])): ?>
    <meta itemprop="closes" content="<?php print $hours['time_end_iso']; ?>"/>
    <?php endif; ?>
  <?php endif; ?>

  <?php
    if (isset($hours['time_start']) && isset($hours['time_end'])) {
      print t('!time_start - !time_end', array('!time_start' => $hours['time_start'], '!time_end' => $hours['time_end']));
    }
    elseif (isset($ĥours['time_start'])) {
      print t('From !time_start', array('!time_start' => $ĥours['time_start']));
    }
    elseif (isset($datetimes['time_end'])) {
      print t('To !time_end', array('!time_end' => $ĥours['time_end']));
    }
  ?>
</div>
<?php endforeach; ?>
<?php endforeach; ?>
