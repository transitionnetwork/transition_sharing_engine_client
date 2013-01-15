<?php
/**
 * @file
 * Provides the administration page for the Tolley CPD module.
 */

/**
 * Administration settings page
 */
function transition_sharing_engine_admin_settings_form(&$form_state) {

  $form['transition_sharing_engine_server_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Sharing Engine server URL'),
    '#description' => t('The url of the Sharing Engine to use - WITHOUT trailing slash.'),
    '#default_value' => variable_get('transition_sharing_engine_server_url', ''),
  );

  //$form['#validate'] = array('tolley_admin_admin_settings_form_validate');
  return system_settings_form($form);
}



/**
 * Validation callback for tolley_admin_admin_settings_form.
 */
function tolley_admin_admin_settings_form_validate($form, &$form_state) {
  if (!is_numeric($form_state['values']['tolley_cpd_notes_points'])) {
    form_set_error('tolley_cpd_notes_points', 'Points values can only be numbers.');
  }
}
