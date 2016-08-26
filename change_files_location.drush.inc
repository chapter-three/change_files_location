<?php

/**
 * @file
 * Change Files Location drush integration.
 *
 * CLI interface to the pages declared by this module.
 */

define('CHANGE_FILE_LOCATION_BATCH_SIZE', 25);


/**
 * Implements hook_drush_command().
 */
function change_files_location_drush_command() {
  $items = array();

  $items['field-file-directory-update'] = array(
    'description' => "Update the file directory for a given field, and move all related files to that new location.",
    'arguments' => array(
      'type'   => 'The type of the entity the field is attached to.',
      'bundle' => 'The bundle this field belongs to.',
      'field'  => 'The name of an existing field.',
    ),
    'required-arguments' => TRUE,
    'options' => array(
      'path' => 'Optional subdirectory within the upload destination where files will be stored. Do not include preceding or trailing slashes. This field supports tokens.',
    ),
    'examples' => array(
      "drush field-file-directory-update node affiliate field_affiliate_image --path='affiliate'" => 'Change the file directory for the field_affiliate_image field of the affiliate content type.',
    ),
    'aliases' => array(
      'fdu',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

/**
 * Verifies and optionally sets user passwords via a csv file.
 *
 * @param string $file
 *   file path to csv file.
 *
 * @see change_files_location_report_access_submit()
 */
function drush_change_files_location_field_file_directory_update($type, $bundle, $field) {
  // @todo Add validation for file directory.
  $file_directory = drush_get_option('path', '');

  // Retrieve field settings and update them.
  $instance = field_read_instance($type, $field, $bundle);
  if (!$instance) {
    drush_set_error("The field instance doesn't exist for the given type, bundle, and field names.");
    return;
  }
  $instance['settings']['file_directory'] = $file_directory;

  field_update_instance($instance);

  // Prepare the new directory.
  $directory = 'public://' . $file_directory;
  file_prepare_directory($directory, FILE_CREATE_DIRECTORY);

  // Load files with old location info.
  $result = db_select("field_data_{$instance['field_name']}", 'f')
    ->fields('f', array("{$instance['field_name']}_fid"))
    ->condition('entity_type', $type)
    ->condition('bundle', $bundle)
    ->execute();

  $fids = $result->fetchAllKeyed(0, 0);
  if ($fids) {
    redirect_run_unprogressive_batch('drush_change_files_location_field_file_directory_update_batch_info', array_keys($fids), $file_directory);
  }
  else {
    drupal_set_message('No files to move.');
  }
}

function drush_change_files_location_field_file_directory_update_batch_info(array $fids, $file_directory = '') {
  $operations[] = array('drush_change_files_location_field_file_directory_update_batch_process', array($fids, $file_directory));

  return array(
    'operations' => $operations,
    'finished' => 'drush_change_files_location_field_file_directory_update_batch_finish',
  );
}

function drush_change_files_location_field_file_directory_update_batch_process(array $fids, $file_directory = '', array &$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = count($fids);
    $context['sandbox']['fids'] = array_values($fids);
  }

  $directory = 'public://' . $file_directory;

  $process_fids = array_slice($context['sandbox']['fids'], $context['sandbox']['progress'], CHANGE_FILE_LOCATION_BATCH_SIZE);
  $files = file_load_multiple($process_fids);
  foreach ($files as $file) {
    $destination = file_stream_wrapper_uri_normalize($directory . '/' . $file->filename);
    file_move($file, $destination);
    $context['results'][] = $file->fid;
  }

  // Update our progress information.
  $context['sandbox']['progress'] += count($process_fids);

  // Inform the batch engine that we are not finished,
  // and provide an estimation of the completion level we reached.
  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = ($context['sandbox']['progress'] >= $context['sandbox']['max']);
  }
}

function drush_change_files_location_field_file_directory_update_batch_finish($success, $results, $operations) {
  if ($success) {
    drupal_set_message(format_plural(count($results), 'One file moved.', '@count files moved.'));
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    drupal_set_message(t('An error occurred while processing @operation with arguments : @args', array('@operation' => $error_operation[0], '@args' => print_r($error_operation[0], TRUE))));
  }
}

/**
 * Perform an unprogressive batch process for CLI.
 */
function change_files_location_run_unprogressive_batch() {
  $batch = batch_get();
  if (!empty($batch)) {
    // If there is already something in the batch, don't run.
    return FALSE;
  }

  $args = func_get_args();
  $batch_callback = array_shift($args);

  if (!lock_acquire($batch_callback)) {
    return FALSE;
  }

  // Attempt to increase the execution time.
  drupal_set_time_limit(240);

  // Build the batch array.
  $batch = call_user_func_array($batch_callback, $args);
  batch_set($batch);

  // We need to manually set the progressive variable again.
  // @todo Remove when http://drupal.org/node/638712 is fixed.
  $batch =& batch_get();
  $batch['progressive'] = FALSE;

  // Run the batch process.
  batch_process();

  lock_release($batch_callback);
  return TRUE;
}
