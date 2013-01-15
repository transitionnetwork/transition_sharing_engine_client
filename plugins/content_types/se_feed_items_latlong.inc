<?php
// $Id:

/**
 * @file
 *   This file provides a CTools content type containing the CPD Position.
 */
/* TODO: handle location context */
/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t('Sharing Engine Items by lat/long'),
  'description' => t('Gets news from the Sharing Engine by source entity, coordinates and proximity.'),
  'single' => TRUE,
  // Constructor & Name of a function which will render the block.
  'content_types' => array('transition_sharing_engine_client_feed_items_latlong'),
  'render callback' => 'transition_sharing_engine_client_feed_items_latlong_content_type_render',
  'edit form' => 'transition_sharing_engine_client_feed_items_latlong_content_type_edit_form',
  'required context' => new ctools_context_required(t('Node'), 'node'),
  'category' => t('Sharing Engine'),
  'defaults' => array(
    'search_type' => '1',
    'search_radius_miles' => '25',
    'search_lat_long' => 'all',
    'items_per_page' => 10,
    'results_pager' => FALSE,
    'results_pager_element' => '0',
    'results_render_style' => 'snippet',
    'results_header' => '',
    'results_header_format' => FILTER_FORMAT_DEFAULT,
  ),
);


/**
 * Output function for the 'tolley_cpd' content type.
 * 
 * Outputs a user's TolleyCPD position for a year, or this year
 */
function transition_sharing_engine_client_feed_items_latlong_content_type_render($subtype, $conf, $panel_args, $context) {
  if (isset($context->data)) $node = $context->data;
  $block = new stdClass();

  // items per page & pager -- SEE: http://api.drupal.org/api/drupal/includes--pager.inc/6/source
  global $pager_page_array, $pager_total, $pager_total_items;
  $page = isset($_GET['page']) ? $_GET['page'] : '0';
  $pager_page_array = explode(',', $page);

  // get title from context node, if present
  if ($node->nid) {
    $block->title = t('Stories from !title', array('!title' => $node->title));
  }
  else {
    $block->title = t('Stories nearby');
  }

  // get latlong and add to path, if needed
  $conf['latlong_query'] = _transition_sharing_engine_client_get_latlong_filter($conf, $node);

  // build the SE server query URL...
  $request_url = transition_sharing_engine_client_service_query_url($conf, $pager_page_array[$conf['results_pager_element']]);
  if ($request_url == '') {
    $block->content = '<p>' . t("Sharing Engine server URL not set up, cannot return news.") . '</p>';
    return $block;
  }

    // query path built, so query!
  $json = drupal_http_request($request_url);
  $decode = json_decode($json->data, TRUE);
  $block->content .= transition_sharing_engine_client_render_items(
    $decode,
    $conf['results_render_style'],
    check_markup($conf['results_header'], $conf['results_header_format'])
  );
  //TODO $block->content = theme('transition_sharing_engine_client', $decode, $conf['items_per_page'], $conf['template_file']);

  // get result count, use to limit pager.
  $result_count = 0;
  if ($decode['feed_items'][0]['item']['results_total']) $result_count = $decode['feed_items'][0]['item']['results_total'];

  // add pager if needed
  if ($conf['results_pager']) {
    // set up pager & 
    $pager_total_items[$conf['results_pager_element']] = $result_count;
    $pager_total[$conf['results_pager_element']] = ceil($pager_total_items[$conf['results_pager_element']] / $conf['items_per_page']);
    $pager_page_array[$conf['results_pager_element']] = max(0, min((int) $pager_page_array[$conf['results_pager_element']], ((int) $pager_total[$conf['results_pager_element']]) - 1));
    $block->content .= theme('pager', NULL, $conf['items_per_page'], 0);
  }

  // add RSS
  drupal_add_feed(transition_sharing_engine_client_service_query_url($conf, null, 'rss'), $block->title . ' RSS feed');

  return $block;
}



/**
 * Returns an edit form for the custom type.
 */
function transition_sharing_engine_client_feed_items_latlong_content_type_edit_form(&$form, &$form_state) {
  $conf = $form_state['conf'];
  $display = &$form_state['display'];
  $plugin = &$form_state['plugin'];

  $contexts = $display->context;
  $form['context'] = ctools_context_selector($contexts, $plugin['required context'], $conf['context']);


  // Max items
  if ($conf['search_type'] == 'all') {
    $entities = array('all');
  }
  else {
    $entities = explode('+', $conf['search_type']);
  }

  $form['search_type'] = array(
    '#type' => 'checkboxes',
    '#title' => t('Entities to query'),
    '#description' => t('The entity types whose feed items should be returned. Choosing all effectively selects all entities.'),
    '#default_value' => $entities,
    '#options' =>  array(
      'all' => 'All',
      '1' => 'Initiatives',
      '4' => 'Partners',
      '3' => 'People',
      '2' => 'Projects',
    ),
  );
  
  // Output style
  $form['results_render_style'] = array(
    '#type' => 'select',
    '#title' => t('Output style'),
    '#description' => t('The style the feed items will be rendered in.'),
    '#default_value' => $conf['results_render_style'],
    '#options' =>  array(
      'snippet' => 'Detailed: Headline, date & body snippet',
      'headlines_source' => 'List: Headline with source',
      'headlines_only' => 'List: Headlines only',
    ),
  );
  
  // Header text
  $form['results_header'] = array(
    '#type' => 'textarea',
    '#title' => t('Header text'),
    '#size' => 20,
    '#description' => t('Enter any explanation or summary here to appear at the top of the results.'),
    '#default_value' => $conf['results_header'],
    '#wysiwyg' => TRUE,
  );
  $form['results_header_format'] = filter_form($conf['results_header_format']);

  // Lat/Long
  $form['search_lat_long'] = array(
    '#type' => 'textfield',
    '#title' => t('Search latitude/longitude'),
    '#size' => 20,
    '#description' => t('Optional coordinates to search around, in the format "latitude,longitude". Only used if no context is chosen, set to "all" to choose all by entity.'),
    '#default_value' => $conf['search_lat_long'],
  );

  // Radius setup
  $form['search_radius_miles'] = array(
    '#type' => 'textfield',
    '#title' => t('Search radius miles'),
    '#size' => 5,
    '#description' => t('How many miles to search around the coordinates.'),
    '#default_value' => $conf['search_radius_miles'],
  );

  // Max items
  $form['items_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Items per page'),
    '#description' => t('The maximum number of items to show.'),
    '#default_value' => $conf['items_per_page'],
    '#options' =>  array(
      '1' => '1',
      '2' => '2',
      '3' => '3',
      '5' => '5',
      '7' => '7',
      '10' => '10',
      '15' => '15',
      '20' => '20',
      '30' => '30',
      '50' => '50',
    ),
  );

    
  // Use pager
  $form['results_pager'] = array(
    '#type' => 'checkbox',
    '#title' => t('Use pager'),
    '#description' => t('Whether to display a pager to request more results or not.'),
    '#default_value' => $conf['results_pager'],
  );

  // Radius setup
  $form['results_pager_element'] = array(
    '#type' => 'textfield',
    '#title' => t('Pager element ID'),
    '#size' => 5,
    '#description' => t('Leave as 0 unless there are several other pagers on this panel.'),
    '#default_value' => $conf['results_pager_element'],
  );
 
}


/**
 * Handles submission of edit form.
 */
function transition_sharing_engine_client_feed_items_latlong_content_type_edit_form_submit(&$form, &$form_state) {
  $form_state['conf']['context'] = $form_state['values']['context'];
  $form_state['conf']['search_lat_long'] = str_replace(' ', '', $form_state['values']['search_lat_long']);
  $form_state['conf']['search_radius_miles'] = (
    is_numeric($form_state['values']['search_radius_miles']) ? $form_state['values']['search_radius_miles'] : 10
  );
  $form_state['conf']['results_render_style'] = $form_state['values']['results_render_style'];
  $form_state['conf']['items_per_page'] = $form_state['values']['items_per_page'];
  $form_state['conf']['results_pager'] = $form_state['values']['results_pager'];
  $form_state['conf']['results_pager_element'] = (
    is_numeric($form_state['values']['results_pager_element']) ? $form_state['values']['results_pager_element'] : 0
  );
  $form_state['conf']['results_header'] = check_markup($form_state['values']['results_header']);
  $form_state['conf']['results_header_format'] = $form_state['values']['results_header_format'];

  // build string for taxonomy IDs on the remote Sharing Engine site...
  $search_type = 'all';
  if (!$form_state['values']['search_type']['all']) { // if 'all' not selected
     $search_type = array();
    foreach ($form_state['values']['search_type'] as $key => $item) {
      if ($item) $search_type[] = $key;
    }
    $search_type = implode('+', $search_type);
  }
  $form_state['conf']['search_type'] = $search_type;
}


/**
 * Helper function to simply get location/radius information from an optional node to be used in the querystring.
 */
function _transition_sharing_engine_client_get_latlong_filter($conf, $node) {
  $latlong = 'all';
  if ($node->nid) {
    // use node's location details for lat/long
    if ($node->location) {
      $latlong = $node->location['latitude'] . ',' . $node->location['longitude'];
      return $latlong . '_' . $conf['search_radius_miles'];
    }
  }
  // node & location unavailable, use panel conf for lat/long
  $latlong = $conf['search_lat_long'];
  if (!$latlong) $latlong = 'all';
  if ($latlong <> 'all') return $latlong . '_' . $conf['search_radius_miles'];
  return 'all';
}
