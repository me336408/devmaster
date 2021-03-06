<?php

/**
 * A simple function to output tasks exactly as we need them.
 *
 * Tired of drupal 6 theme system, going as fast as I can.
 *
 * @param $tasks
 *
 * Usage:
 * $tasks = hosting_get_tasks('task_status', HOSTING_TASK_PROCESSING);
 * print boots_render_tasks($tasks);
 */
function boots_render_tasks($tasks = NULL, $class = '', $actions = array()){
  global $user;

  if (is_null($tasks)){
    // Tasks
    $tasks = hosting_get_tasks(null, null, 10);
  }

  // Get active or queued
  $tasks_count = 0;
  foreach ($tasks as $task){
    if ($task->task_status == HOSTING_TASK_QUEUED || $task->task_status == HOSTING_TASK_PROCESSING){
      $tasks_count++;
    }
  }

  if ($tasks_count > 0) {
    $task_class = 'active-task fa-spin';
  }

  $items = array();
  $text = '<i class="fa fa-list-alt"></i> '. t('Task Logs');

  // If for an environment, change the link.
  if (!empty($actions)) {

    $environment_node = node_load($tasks[0]->rid);
    $environment = $environment_node->environment;

    $url = "node/{$environment->project_nid}/logs/{$environment->name}";
  }
  else {
    $url = 'hosting/queues/tasks';
  }

  $task_items = array();
  $task_items[] = l($text, $url, array(
    'html' => TRUE,
    'attributes' => array(
      'class' => 'list-group-item',
    ),
  ));

  $task_types = hosting_available_tasks();

  if (!empty($tasks)) {

    foreach ($tasks as $task) {

      switch ($task->task_status){
        case HOSTING_TASK_SUCCESS:
          $icon = 'check text-success';
          $item_class = 'list-group-item-success';
          break;

        case HOSTING_TASK_ERROR;
          $icon = 'exclamation-circle text-danger';
          $item_class = 'list-group-item-danger';
          break;
        case HOSTING_TASK_WARNING:
          $icon = 'warning text-warning';
          $item_class = 'list-group-item-warning';
          break;

        case HOSTING_TASK_PROCESSING;
        case HOSTING_TASK_QUEUED;
          $icon = 'cog fa-spin text-info';
          $item_class = 'list-group-item-info';
          break;
      }

      $task_node = node_load($task->rid);

      // If environment tasks...
      if (!empty($actions)) {
        $task->title = $task_types[$task_node->type][$task->task_type]['title'];
      }

      $text = '<i class="fa fa-' . $icon . '"></i> ';
      $text .= $task->title;
      $text .= ' <small class="btn-block">' . format_interval(time() - $task->changed) .' '. t('ago') . '</small>';

      $task_items[] = l($text, 'node/' . $task->nid, array(
        'html' => TRUE,
        'attributes' => array(
          'class' => 'list-group-item ' . $item_class,
        ),
      ));
    }
  }
  else {
    $task_items[] = t('No Active Tasks');
  }

  $items[] = array(
    'class' => 'tasks',
    'data' => '<div class="list-group">' . implode("\n", $task_items) . '</div>',
  );

  if (!empty($actions)) {

    array_unshift($items, array(
      'class' => 'divider',
    ));
    array_unshift($items, l('<i class="fa fa-sliders"></i> ' . t('Environment Settings'), "node/{$environment->project_nid}/edit/{$environment->name}", array('html' => TRUE)));

    $action_items = array();
    foreach ($actions as $link) {
      $action_items[] = l($link['title'], $link['href'], array(
        'attributes' => array(
          'class' => 'list-group-item',
        ),
        'query' => array(
          'token' => drupal_get_token($user->uid),
        ),
      ));
    }

    $items[] = array(
      'class' => 'actions',
      'data' => '<div class="list-group">' . implode("\n", $action_items) . '</div>',
    );
  }

  $tasks = theme('item_list', $items, '', 'ul', array('class' => 'devshop-tasks dropdown-menu dropdown-menu-right', 'role' => 'menu'));

  if ($tasks_count == 0) {
    $tasks_count = '';
  }

  $logs = t('Task Logs');
  return <<<HTML
    <div class="task-list btn-group">
      <button type="button" class="btn btn-link task-list-button dropdown-toggle $class" data-toggle="dropdown" title="$logs">
          $tasks_count
        <i class="fa fa-gear $task_class"></i>
      </button>
      $tasks
    </div>
HTML;

}

/**
 * Implements hook_preprocess_page()
 * @param $vars
 */
function boots_preprocess_page(&$vars){

  if (user_access('access task logs')){
    $vars['tasks'] = boots_render_tasks();
  }

  if ($vars['node']) {

    // Removing conflicting scripts.
    // Not sure how this actually works.  Drupal 6 is fun!
    // Thanks, http://drupal.stackexchange.com/questions/5076/remove-every-javascript-except-own-theme-scripts
    drupal_add_js(path_to_theme(). '/js/bootstrap.min.js', 'theme');
    $js = drupal_add_js();
    unset($js['core']);
    unset($js['module']);
    $vars['scripts'] = $js;

    // Set subtitle
    if ($vars['node']->type == 'project'){
      $vars['subtitle'] = t('Project');

      unset($vars['tabs']);

      $vars['title'] = l($vars['title'], "node/" . $vars['node']->nid);
    }
  }
}


/**
 *
 * @param $vars
 */
function boots_preprocess_node(&$vars) {
  global $user;
  if ($vars['node']->type == 'project') {
    boots_preprocess_node_project($vars);
  }
  elseif ($vars['node']->type == 'task') {
    boots_preprocess_node_task($vars);
  }
}

/**
 * Preprocessor for Project Nodes.
 * @param $vars
 */
function boots_preprocess_node_task(&$vars) {

  $revisions = node_revision_list($vars['node']);

  $revision = array_shift($revisions);

  $vars['submitted'] = t('Task Queued by !user, @time (%ago)', array(
    '!user' => l($revision->name, "user/$revision->uid"),
    '@time' => format_date($revision->timestamp),
    '%ago' => format_interval(time() - $revision->timestamp) . ' ' . t('ago'),
  ));

}
/**
 * Preprocessor for Project Nodes.
 * @param $vars
 */
function boots_preprocess_node_project(&$vars){
  global $user;

  // Easy Access
  $node = &$vars['node'];
  $project = $vars['project'] = $vars['node']->project;

  // Live Domain link.
  if ($project->settings->live['live_domain']) {
    $vars['live_domain_url'] =  'http://' . $project->settings->live['live_domain'];
    $vars['live_domain_text'] =  'http://' . $project->settings->live['live_domain'];
  }
  else {
    $vars['live_domain_url'] =  '';
  }

  $vars['git_refs'] = array();

  if (empty($node->project->settings->git['refs'])){
    $vars['deploy_label'] = '';

    if ($node->verify->task_status == HOSTING_TASK_ERROR) {
      $vars['deploy_label'] = t('There was a problem refreshing branches and tags.');
      $vars['git_refs'][] = l(t('View task log'), 'node/' . $node->verify->nid);
      $vars['git_refs'][] = l(t('Refresh branches'), 'node/' . $node->nid . '/project_verify', array('attributes' => array('class' => 'refresh-link'), 'query' => array('token' => drupal_get_token($user->uid))));
    }
    elseif ($node->verify->task_status == HOSTING_TASK_QUEUED || $node->verify->task_status == HOSTING_TASK_PROCESSING) {
      $vars['deploy_label'] =  t('Branches refreshing.  Please wait.');
    }
  }
  else {
    $vars['deploy_label'] = t('Deploy a tag or branch');

    foreach ($node->project->settings->git['refs'] as $ref => $type){
      $href = url('node/ENV_NID/site_devshop-deploy', array(
        'query' =>array(
          'git_ref' => $ref,
        )
      ));
      $icon = $type == 'tag'? 'tag': 'code-fork';

      $vars['git_refs'][$ref] = "<a href='$href'>
        <i class='fa fa-$icon'></i>
        $ref
      </a>";
    }
  }

  // Get available servers
  $vars['web_servers'] = hosting_get_servers('http');
  $vars['db_servers'] = hosting_get_servers('db');

  // React to git provider
  if ($project->git_provider == 'github') {
    $url = strtr($project->git_url, array(
      'git@github.com:' => 'http://github.com/',
      '.git' => '',
    ));
    if (empty($project->settings->deploy['last_webhook'])){
      $url .= '/settings/hooks/new';
    }
    else {
      $url .= '/settings/hooks';
    }
    $vars['add_webhook_url'] = $url;
    $vars['add_webhook_icon'] = 'github';
  }
  else {
    $vars['add_webhook_url'] = '#';
    $vars['add_webhook_icon'] = 'warning';
  }

  // Set webhook interval
  if ($project->settings->deploy['method'] == 'webhook' && $project->settings->deploy['last_webhook']){
    $interval = format_interval(time() - $project->settings->deploy['last_webhook']);
    $vars['webhook_ago'] = t('@time ago', array('@time' => $interval));
  }

  if ($project->settings->deploy['method'] == 'queue') {
    $vars['queued_ago'] = hosting_format_interval(variable_get('hosting_queue_deploy_last_run', FALSE));
  }

  // Webhook status output.
  if (empty($node->project->settings->deploy['last_webhook'])) {
    $button_text = t('Setup Webhook');
    $class = 'btn-warning';
  }
  else {
    $button_text =  t('Webhook URL');
    $class = 'text-muted';
  }

  $title = t('Webhook for project %name', array('%name' => $node->title));
  $prefix = t('Deploy code to your environments with an incoming webhook with the following URL:');

  if ($project->git_provider == 'github') {
    $suffix = t('GitHub will ping this URL after each code push to keep the servers up to date, and can create environments on Pull Request.');
    $suffix2 = t('Copy the link above, then click the link below to go to the webhooks page for this project.');
    $suffix3 = t('DevShop only has support for Push and Pull Request events.  Set content type to <em>application/json</em>.');

    if (empty($project->settings->deploy['last_webhook'])){
      $github_button_text = t('Add a Webhook at GitHub.com');
    }
    else {
      $github_button_text = t('Manage Webhooks at GitHub.com');
    }
    $button = l($github_button_text, $vars['add_webhook_url'], array('attributes'=> array('class' => 'btn btn-primary', 'target' => '_blank')));
  }
  else {
    $suffix = t('Ping this URL after each code push to keep the servers up to date.');
    $button = '';
    //@TODO: Link to more help such as example scripts.
  }

  $url =  $node->project->webhook_url;
  $project_name = $node->title;
  $vars['webhook_url'] = <<<HTML

            <a href="#" data-toggle="modal" class="btn btn-xs $class"
data-target="#webhook-modal" title="Webhook URL">
              <i class="fa fa-chain"></i> $button_text
            </a>

            <!-- Modal -->
            <div class="modal fade" id="webhook-modal" tabindex="-1" role="dialog" aria-labelledby="webhook-modal" aria-hidden="true">
              <div class="modal-dialog">
                <div class="modal-content">
                  <div class="modal-header">
                    <button type="button" class="close" data-dismiss="modal"><span aria-hidden="true">&times;</span><span class="sr-only">Close</span></button>
                    <h4 class="modal-title" id="drush-alias-modal">$title</h4>
                  </div>
                  <div class="modal-body">
                  <p>
                    $prefix
                  </p>
                  <p><input class="form-control" value="$url" onclick="this.select()"></p>
                  <p>
                    $suffix
                  </p>
                  <p>
                    $suffix2
                  </p>
                  <p>
                    $suffix3
                  </p>
                  $button
                  </div>
                  <div class="modal-footer">
                    <button type="button" class="btn btn-default" data-dismiss="modal">Close</button>
                  </div>
                </div>
              </div>
            </div>
HTML;

  $vars['hosting_queue_admin_link'] = l(t('Configure Queues'), 'admin/hosting/queues');

  // Available deploy data targets.
  $vars['target_environments'];

  foreach ($vars['node']->project->environments as &$environment) {

    // Environment Tasks
    if ($environment->site) {
      $environment->tasks = hosting_get_tasks('rid', $environment->site);
    }
    else {
      $environment->tasks = hosting_get_tasks('rid', $environment->platform);
    }

    $environment->task_count = count($environment->tasks);
    $environment->active_tasks = 0;
    $environment->tasks_list = boots_render_tasks($environment->tasks, 'environment btn btn-small btn-link', $node->environment_actions[$environment->name]);

    foreach ($environment->tasks as &$task) {
      if ($task->task_status == HOSTING_TASK_QUEUED || $task->task_status == HOSTING_TASK_PROCESSING) {
        $environment->active_tasks++;
      }
    }

    if ($environment->site) {
      $vars['source_environments'][$environment->name] = $environment;
    }

    // Status
    if ($environment->site_status == HOSTING_SITE_DISABLED) {
      $environment->class = 'disabled';
      $environment->list_item_class = 'disabled';
    }
    elseif ($environment->name == $project->settings->live['live_environment']) {
      $environment->class = ' live-environment';
      $environment->list_item_class = 'info';
    }
    else {
      $environment->class = ' normal-environment';
      $environment->list_item_class = 'info';
    }

    // Active?
    if ($environment->active_tasks > 0) {
      $environment->class .= ' active';
      $environment->list_item_class = 'warning';
    }

    $environment->active_tasks_label = format_plural(
      $environment->active_tasks,
      t('1 active task'),
      t('@count active tasks', array('@count' => $environment->active_tasks))
    );

    // Get login link
    // @TODO: This is how aegir does it.  See _hosting_site_goto_link()
    // @TODO: Detect and display "Generating login" message.
    if ($environment->site_status == HOSTING_SITE_ENABLED) {
      $cache = cache_get("hosting:site:" . $environment->site . ":login_link");
      if ($cache && (time() < $cache->data['expire'])) {
        $environment->login_url = url("node/" . $environment->site . "/goto_site");
        $environment->login_text = t('Log in');
      }
      else {
        $environment->login_url = url("node/{$environment->site}/site_login-reset", array('query' => array('token' => drupal_get_token($user->uid))));
        $environment->login_text = t('Request Login');

        $task = hosting_get_most_recent_task($environment->site, 'login-reset');
        if (!empty($task) && $task->task_status == HOSTING_TASK_QUEUED || $task->task_status == HOSTING_TASK_PROCESSING) {
          $environment->login_text = t('Please Wait...');
          $environment->login_url = '#';
        }
      }
    }
  }
}

/**
 * Override for item_list
 */
function boots_item_list($items = array(), $title = NULL, $type = 'ul', $attributes = NULL) {
  $output = '';
  if (!empty($title)) {
    $output .= '<h3>' . $title . '</h3>';
  }

  if (!empty($items)) {
    $output .= "<$type" . drupal_attributes($attributes) . '>';
    $num_items = count($items);
    foreach ($items as $i => $item) {
      $attributes = array();
      $children = array();
      if (is_array($item)) {
        foreach ($item as $key => $value) {
          if ($key == 'data') {
            $data = $value;
          }
          elseif ($key == 'children') {
            $children = $value;
          }
          else {
            $attributes[$key] = $value;
          }
        }
      }
      else {
        $data = $item;
      }
      if (count($children) > 0) {
        $data .= theme_item_list($children, NULL, $type, $attributes); // Render nested list
      }
      if ($i == 0) {
        $attributes['class'] = empty($attributes['class']) ? 'first' : ($attributes['class'] . ' first');
      }
      if ($i == $num_items - 1) {
        $attributes['class'] = empty($attributes['class']) ? 'last' : ($attributes['class'] . ' last');
      }
      $output .= '<li' . drupal_attributes($attributes) . '>' . $data . "</li>\n";
    }
    $output .= "</$type>";
  }
  return $output;
}

/**
 * Implements hook_status_messages()
 */
function boots_status_messages($display = NULL) {
  $output = '';
  foreach (drupal_get_messages($display) as $type => $messages) {
    if ($type == 'status') {
      $class = "alert alert-success";
    }
    elseif ($type == 'warning') {
      $class = "alert alert-warning";
    }
    elseif ($type == 'error') {
      $class = "alert alert-danger";
    }

    $output .= "<div class=\"$class\">\n";
    if (count($messages) > 1) {
      $output .= " <ul>\n";
      foreach ($messages as $message) {
        $output .= '  <li>' . $message . "</li>\n";
      }
      $output .= " </ul>\n";
    }
    else {
      $output .= $messages[0];
    }
    $output .= "</div>\n";
  }
  return $output;
}