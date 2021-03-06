<?php
/**
 * @file devshop_project.form.inc DevShop Project forms related hooks and
 * support function include file.
 */

/**
 * Implements hook_form_alter().
 */
function devshop_projects_form_alter(&$form, &$form_state, $form_id){

  // Removing unneccesary fieldgroups
  if ($form_id == 'project_node_form'){
    global $user;

    $project = $form['#node'];

    unset($form['menu']);
    unset($form['revision_information']);
    unset($form['author']);
    unset($form['options']);
    unset($form['buttons']['delete']);
    unset($form['buttons']['preview']);
    if ($form['retry']['#value']) {
      $form['buttons']['submit']['#value'] = t('Save and Retry');
    }

    //Add button for delete project
    $form['buttons']['delete'] = array(
      '#type' => 'markup',
      '#value' => l(t('Delete this project'), 'node/'. $project->nid .'/project_delete', array('query' => array('token' => drupal_get_token($user->uid)))),
      '#weight' => 10,
    );
  }

  // Create Project Wizard
  if ($form_id == 'devshop_project_create_step_sites' || $form_id == 'devshop_project_create_step_settings' || $form_id == 'devshop_project_create_step_environments'){
    if ($form_state['project']->no_finish){
      unset($form['buttons']['return']);
    }
     if ($form_state['project']->no_next){
      unset($form['buttons']['next']);
    }
    $form['buttons']['next']['#attributes'] =
    $form['buttons']['return']['#attributes'] = array(
      'class' => 'btn btn-success',
    );
    $form['buttons']['previous']['#attributes'] = array(
      'class' => 'btn btn-default',
    );
    $form['buttons']['cancel']['#attributes'] = array(
      'class' => 'btn btn-link',
    );

  }
  
  // Hosting Task Forms
  if ($form_id == 'hosting_task_confirm_form') {
    switch($form['task']['#value']) {

      // Create Environment form.
      case 'devshop-create':

        if (arg(3) == 'clone') {
          drupal_set_title(t('Clone environment'));
        }
        elseif (arg(3) == 'fork') {
          drupal_set_title(t('Fork environment'));
        }
        else {
          drupal_set_title(t('Create new environment'));
        }
        $form['#submit'] = array('hosting_task_devshop_create_form_submit');
        break;

      // Migrate Form: used for changing database server.
      case 'migrate':

        // To change the database server, we use the migrate form.
        if ($_GET['deploy'] == 'stack') {
          drupal_set_title(t('Change Database Server'));
          $site_node = node_load($form['nid']['#value']);
          $environment = $site_node->environment;
          $form['help']['#weight'] = 100;
          $form['help']['#value'] = t("Are you sure you want to change this environment's database server?");

          // Set URI value
          $form['parameters']['new_uri']['#type'] = 'value';
          $form['parameters']['new_uri']['#value'] = $form['parameters']['new_uri']['#default_value'];

          // Display something helpful
          $form['old'] = array(
            '#type' => 'item',
            '#title' => t('Current Database Server'),
            '#value' => l($environment->servers['db']['name'], 'hosting/c/server_' . $environment->servers['db']['nid']),
            '#weight' => '-1',
          );

          foreach ($form['parameters'] as $key => &$element) {
            if (is_int($key)) {
              unset($form['parameters'][$key]);
            }
          }
          $form['parameters']['#suffix'] = '</div>';
        }
        break;

      // Deploy task form
      case 'devshop-deploy':

        // Alter title of deploy task.
        $node = node_load($form['nid']['#value']);
        drupal_set_title(t('Deploy code to Environment "@env"', array('@env' => $node->environment->name)));
        $form['actions']['cancel']['#value'] = l(t('Cancel'), "node/{$node->project->nid}");
        break;

      // Sync task form
      case 'sync':
        if (isset($_GET['source'])) {
          $node = node_load($form['nid']['#value']);

          if ($_GET['source'] == 'other') {
            $source = '';
          }
          else {
            $source = node_load($_GET['source']);
            if (isset($source->nid) && $source->type != 'site') {
              break;
            }
          }

          drupal_set_title(t('Deploy data to Environment "@env"', array('@env' => $node->environment->name)));
          $environment = $node->environment;

          $form['help'] = array(
            '#prefix' => '<div class="alert alert-warning" role="alert">',
            '#suffix' => '</div>',

            'target' => array(
              '#value' => t('Target: ') . l($environment->url, $environment->url, array('attributes' => array('target' => '_blank'))) . ' <span><i class="fa fa-warning"></i> ' . t('Data will be destroyed.') . '</span>',
              '#prefix' => '<p class="text-danger">',
              '#suffix' => '</p>',
            ),
          );

          // If source is empty, ask for an alias.
          if (empty($source)) {
            $form['parameters']['source'] = array(
              '#type' => 'textfield',
              '#title' => t('Drush Alias for Source'),
              '#description' => t('Enter a full drush alias to use as the source. Make sure to include the "@" character at the beginning.'),
            );
          }
          // If a source is specified, add help and set it as a value.
          else {

            $source_environment = $source->environment;
            $form['help']['source'] = array(
              '#value' => t('Source: ') . l($source_environment->url, $source_environment->url, array('attributes' => array('target' => '_blank'))),
              '#prefix' => '<p>',
              '#suffix' => '</p>',
            );

            $form['parameters']['source'] = array(
              '#type' => 'value',
              '#value' => $source->environment->drush_alias,
            );
          }

          // Don't copy modules and themes.
          unset($form['parameters']['modules']);
          unset($form['parameters']['themes']);
          unset($form['parameters']['libraries']);

          // Better output
          $form['parameters']['database']['#prefix'] = '<label>' . t('What to Deploy?') . '</label>';
          $form['parameters']['registry-rebuild']['#prefix'] = '<label>' . t('Run Deploy Hooks?') . '</label>';
          $form['parameters']['backup']['#weight'] = -11;

          $form['actions']['submit']['#value'] = t('Deploy Data');
          $form['actions']['cancel']['#value'] = l(t('Cancel'), "node/{$node->project->nid}");

          $form['warning']['#value'] = '<div class="alert alert-spacing alert-danger">' . t('Clicking "Deploy Data" will DESTROY the database for the environment !link.', array('!link' => l($environment->url, $environment->url, array('attributes' => array('target' => '_blank'))))) . '</div>';
        }
        break;
    }
  }

  // Platform "Edit" page.
  if ($form_id == 'platform_node_form') {

    // Set to values
    $form['title']['#type'] = 'value';
    $form['makefile']['#type'] = 'value';
    unset($form['info']);

    // If switching webservers...
    if ($_GET['web_server']){

      $platform_node = node_load($form['nid']['#value']);
      $environment = $platform_node->environment;
      $form['help'] = array(
        '#value' => t("Are you sure you want to change this site's web server?  <strong>NOTE: You will have to change DNS as well!</strong>"),
        '#weight' => -10,
      );

      $web_server = $_GET['web_server'];
      $web_server_nid = db_result(db_query('SELECT nid FROM {node} WHERE type = "server" && title = "%s"', $web_server));

      // Set values
      $form['web_server']['#default_value'] = $web_server_nid;
      $form['web_server']['#type'] = 'hidden';

      // Load servers to display IP addresses.
      $old_server = node_load($environment->servers['http']['nid']);
      $new_server = node_load($web_server_nid);

      // Display something helpful
      $form['old'] = array(
        '#type' => 'item',
        '#title' => t('Current Web Server'),
        '#value' => l($environment->servers['http']['name'], 'node/' . $environment->servers['http']['nid']) . '<br />' . implode($old_server->ip_addresses, '<br />'),
        '#weight' => '-1',
      );

      // Display something helpful
      $form['new'] = array(
        '#type' => 'item',
        '#title' => t('New Web Server'),
        '#value' => l($web_server, "node/$web_server_nid") . '<br />' . implode($new_server->ip_addresses, '<br />'),
        '#weight' => '0',
      );
    }
  }

  // Site "Edit" page.  This is our environment settings page..
  if ($form_id == 'site_node_form') {

    $node = $form['#node'];
    $environment = $node->environment;

    // Remove aegir's helpful info panel.
    unset($form['info']);

    // Help Text
    $form['note'] = array(
      '#value' => t('These settings apply only to the environment !site', array('!site' => l($environment->name, $environment->url))),      '#weight' => -10,
    );

    $form['environment'] = array(
      '#weight' => -9,
      '#tree' => true,
    );

    // Values
    $form['environment']['project_nid'] = array(
      '#value' => $environment->project_nid,
      '#type' => 'value',
    );
    $form['environment']['project_name'] = array(
      '#value' => $environment->project_name,
      '#type' => 'value',
    );
    $form['environment']['name'] = array(
      '#value' => $environment->name,
      '#type' => 'value',
    );
    $form['environment']['git_ref'] = array(
      '#value' => $environment->git_ref,
      '#type' => 'value',
    );
    $form['environment']['site'] = array(
      '#value' => $environment->site,
      '#type' => 'value',
    );
    $form['environment']['platform'] = array(
      '#value' => $environment->platform,
      '#type' => 'value',
    );

    // User-configurable Settings
    $form['environment']['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Environment Settings'),
      '#tree' => true,
    );
    $form['environment']['settings']['locked'] = array(
      '#type' => 'checkbox',
      '#title' => t('Lock Database'),
      '#default_value' => $environment->settings->locked,
      '#description' => t('Prevent devshop users from destroying the database.') . '<p class="text-danger">Drush users may still overwrite the database.</p>',
    );

    $form['environment']['settings']['pull_disabled'] = array(
      '#type' => 'checkbox',
      '#title' => t('Disable Deploy on Commit'),
      '#default_value' => $environment->settings->pull_disabled,
      '#description' => t('Do not pull code to the server on commit & push.'),
    );

    $form['environment']['settings']['deploy'] = array(
      '#type' => 'fieldset',
      '#title' => t('Deployment Hooks'),
      '#description' => t('When code is deployed to this environment, do the following:'),
    );
    $form['environment']['settings']['deploy']['revert'] = array(
      '#type' => 'checkbox',
      '#title' => t('Revert all features.'),
      '#description' => t('If features is enabled, revert all of them.'),
      '#default_value' => empty($node->nid)? 1: $environment->settings->deploy['revert'],
    );
    $form['environment']['settings']['deploy']['update'] = array(
      '#type' => 'checkbox',
      '#title' => t('Run database updates.'),
      '#default_value' => empty($node->nid)? 1: $environment->settings->deploy['update'],
    );
    $form['environment']['settings']['deploy']['cache'] = array(
      '#type' => 'checkbox',
      '#title' => t('Clear all caches.'),
      '#default_value' => empty($node->nid)? 1: $environment->settings->deploy['cache'],
    );

    // If acquia cloud hooks are detected, add the option to fire them.
    if (file_exists($environment->repo_root . '/hooks/common/post-code-update')) {
      $form['environment']['settings']['deploy']['acquia_hooks'] = array(
        '#type' => 'checkbox',
        '#title' => t('Run Acquia "post-code-update" hook scripts for environment "%env".', array('%env' => $environment->name)),
        '#description' => 'Cloud hooks were detected in your project.  Check this box to run the "post-code-update" hook every time you deploy code to this environment.',
        '#default_value' => empty($node->nid)? 0: $environment->settings->deploy['acquia_hooks'],
      );
    }

    // Add our own submit handler
    $form['buttons']['submit']['#submit'][] = 'devshop_projects_environment_settings_submit';
  }
}

/**
 * Submit handler for site/environment settings page.
 */
function devshop_projects_environment_settings_submit($form, &$form_state){

  // Save the environment record.
  // @TODO: Replace with an $environment->save();

  // Ensure correct data types
  $environment = (object) $form_state['values']['environment'];
  $environment->settings = (array) $environment->settings;

  // Prepare record for saving
  $info = new stdClass();
  $info->project_nid = $environment->project_nid;
  $info->name = $environment->name;
  $info->git_ref = $environment->git_ref;
  $info->site = $environment->site;
  $info->platform = $environment->platform;
  $info->settings = serialize($environment->settings);

  // Save environment record.
  if (drupal_write_record('hosting_devshop_project_environment', $info, array('project_nid', 'name'))){

    // Remove the "Site dev.drupal.devshop.local has been updated.
    drupal_get_messages('status');

    // Send our own message
    drupal_set_message(t('Environment settings saved for %env in project %project', array('%env' => $environment->name, '%project' => $environment->project_name)));
  }
  else {
    watchdog('error', 'Environment record not saved: ' . print_r($info, 1));
  }

  $form_state['redirect'] = 'node/' . $environment->project_nid;
}


/**
 * Implementation of hook_form().
 */
function devshop_projects_form(&$node) {

  $project = $node->project;

  // Save last project data
  $form['old'] = array(
    '#value' => $node,
    '#type' => 'value',
  );

  // Project Settings
  // Every value under $form['project'] gets serialized and saved into a project's "data" column.
  $form['project'] = array(
    '#tree' => TRUE,
  );

  // Hidden fields that can't change.
  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Project Code Name'),
    '#required' => TRUE,
    '#description' => t('Choose a unique name for your project.'),
    '#size' => 40,
    '#default_value' => $node->title,
    '#maxlength' => 255,
  );
  $form['project']['git_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Git URL'),
    '#required' => TRUE,
    '#description' => t(''),
    '#required' => TRUE,
    '#size' => 40,
    '#default_value' => $project->git_url,
    '#maxlength' => 255,
    '#weight' => 1,
  );
  $form['project']['code_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Code path'),
    '#description' => t('The absolute path on the filesystem that will be used to create the platform in the directory specified above.'),
    '#required' => TRUE,
    '#size' => 40,
    '#default_value' => $project->code_path,
    '#maxlength' => 255,
    '#weight' => 2,
  );
  $form['project']['drupal_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Path to Drupal'),
    '#description' => t('Enter the relative path to the index.php file in your repository. Leave blank if index.php is in the root.'),
    '#size' => 40,
    '#default_value' => $project->drupal_path,
    '#maxlength' => 255,
    '#weight' => 3,
  );
  $form['project']['base_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Primary Domain'),
    '#description' => t('The domain name all sites will be built under.'),
    '#required' => TRUE,
    '#size' => 40,
    '#default_value' => $project->base_url,
    '#maxlength' => 255,
    '#weight' => 4,
  );

  // Don't allow editing
  if ($node->nid) {

    // Title
    $form['title']['#value'] = $form['title']['#default_value'];
    $form['title']['#type'] = 'value';

    // Other fields
    $locked = array('git_url', 'code_path', 'drupal_path', 'base_url');
    foreach ($locked as $field){
      $form['project'][$field]['#value'] = $form['project'][$field]['#default_value'];
      $form['project'][$field]['#type'] = 'value';
    }
  }

  // Project Settings
  // Save git branches and tags
  $form['project']['settings']['git']['branches'] = array(
    '#type' => 'value',
    '#value' => $project->settings->git['branches'],
  );
  $form['project']['settings']['git']['tags'] = array(
    '#type' => 'value',
    '#value' => $project->settings->git['tags'],
  );

  // Live Environment settings.
  $form['project']['settings']['live'] = array(
    '#type' => 'fieldset',
    '#title' => t('Domain Name Settings'),
  );

  // Live Environment
  $environments = array_keys($project->environments);
  if (empty($environments)){
    $environments_options= array();
  }
  else {
    $environments_options = array_combine($environments, $environments);
    $environments_options[''] = t('No live environment');
  }

  $form['project']['settings']['live']['live_environment'] = array(
    '#type' => 'select',
    '#title' => t('Live environment'),
    '#description' => t('The environment for the live website.  This locks the live environment from overwrites and disabling, and allows syncing to other environments.'),
    '#options' => $environments_options,
    '#default_value' => $project->settings->live['live_environment'],
  );

  // Live Domain
  $form['project']['settings']['live']['live_domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Live Domain'),
    '#description' => t('The live domain for this project. Used only for links and when creating subdomain aliases for other environments. <em>You must still add the Domain to your live environment manually.</em>'),
    '#size' => 40,
    '#default_value' => $node->project->settings->live['live_domain'],
  );

  // Use aliases
  $form['project']['settings']['live']['environment_aliases'] = array(
    '#type' => 'checkbox',
    '#title' => t('For new environments, create subdomains under Live Domain.'),
    '#description' => t('When new environments are created, automatically add a domain name such as http://ENVIRONMENT.LIVEDOMAIN.com. <em>Does not affect existing environments. Does not remove domains when disabled.</em>'),
    '#default_value' => $project->settings->live['environment_aliases'],
  );

  // Pull Code Method
  $form['project']['settings']['deploy'] = array(
    '#type' => 'fieldset',
    '#title' => t('Deploy Settings'),
    '#description' => t('Configure how code is delivered to the servers. Post Deploy hooks are configured per environment.'),
  );

  $form['project']['settings']['deploy']['method'] = array(
    '#title' => 'Deploy Code Method',
    '#type' => 'radios',
    '#description' => t('Choose the method used to deploy code to the server.'),
    '#default_value' => $project->settings->deploy['method'],
  );

  // Commit Webhook
  $form['project']['settings']['deploy']['method']['#options']['webhook'] = t('Immediate Deployment');
  $form['project']['settings']['deploy']['method']['#options']['webhook'] .= '<div class="description">' . t('Recommended. Deploy code as it is delivered to your repository.') . ' <em>' . t('Requires setting up a webhook with your git repository host.') . '</em>' . '</div>';

  // Queue
  $queues = hosting_get_queues();
  if ($queues['deploy']['enabled']) {
    $form['project']['settings']['deploy']['method']['#options']['queue'] = t('Queued Deployment');

    $t = array();
    $t['@freq'] = format_interval($queues['deploy']['frequency'], 1);

    $form['project']['settings']['deploy']['method']['#options']['queue'] .= '<div class="description">';
    $form['project']['settings']['deploy']['method']['#options']['queue'] .= t('Deploy code every @freq. Only runs deploy hooks if code has changed.  Use only if repository webhooks are not available.', $t);


    if (user_access('administer hosting queues')) {
      $form['project']['settings']['deploy']['method']['#options']['queue'] .= ' '. l(t("Deploy Queue configured to run every @freq.", $t), 'admin/hosting/queues');
    } else {
      $form['project']['settings']['deploy']['method']['#options']['queue'] .= ' '. t("Deploy Queue configured to run every @freq.", $t);

    }

    $form['project']['settings']['deploy']['method']['#options']['queue'] .= '</div>';

  }

  // Manual Pull
  $form['project']['settings']['deploy']['method']['#options']['manual'] = t('Manual Deployment');
  $form['project']['settings']['deploy']['method']['#options']['manual'] .= '<div class="description">' . t('Deploy code to servers manually via devshop or drush.');

  $form['project']['settings']['deploy']['method']['#options']['manual'] .= ' <em>' . t('Not recommended. All environments must be manually updated.') . '</em>'. '</div>';

  // Add link to hosting queues admin if the user can access them.
  if (!$queues['deploy']['enabled'] && user_access('administer hosting queues')){
    $form['project']['settings']['deploy']['queue_admin'] = array(
      '#value' => t('The !link is disabled. Enable it to allow projects to pull code in the queue.', array(
        '!link' => l(t('Pull Queue'), 'admin/hosting/queues'),
      )),
      '#prefix' => '<p>',
      '#suffix' => '</p>',
    );
  }

  // Allow "Other"
  $form['project']['settings']['deploy']['allow_deploy_data_from_alias'] = array(
    '#type' => 'checkbox',
    '#title' => t('Allow deploying data from drush aliases.'),
    '#default_value' => $node->project->settings->deploy['allow_deploy_data_from_alias'],
    '#description' => t('Requires a patch'),
  );

  // @TODO: is there a better way to save certain values? We lose data without these.
  $form['project']['settings']['deploy']['last_webhook'] = array(
    '#type' => 'value',
    '#value' => $node->project->settings->deploy['last_webhook'],
  );
  $form['project']['settings']['deploy']['last_webhook_status'] = array(
    '#type' => 'value',
    '#value' => $node->project->settings->deploy['last_webhook_status'],
  );
  $form['project']['settings']['deploy']['last_webhook_ip'] = array(
    '#type' => 'value',
    '#value' => $node->project->settings->deploy['last_webhook_ip'],
  );

  // @TODO: Make this more abstract and extensible.
  $is_github = (strpos($node->project->git_url, 'github.com') !== FALSE);

  //All settings git pull in project page
  $form['project']['settings']['github'] = array(
    '#type' => 'fieldset',
    '#title' => t('GitHub Integration'),
    '#access' => $is_github,
  );

  // Pull Requests create environments?
  $form['project']['settings']['github']['pull_request_environments'] = array(
    '#type' => 'checkbox',
    '#title' => t('Create Environments for Pull Requests'),
    '#default_value' => $node->project->settings->github['pull_request_environments'],
    '#description' => t('If using GitHub, create a new environment when a new Pull Request is created.'),
    '#access' => $is_github,
  );

  // Delete Pull Request environments?
  $form['project']['settings']['github']['pull_request_environments_delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete Pull Request Environments'),
    '#default_value' => $node->project->settings->github['pull_request_environments_delete'],
    '#description' => t('When Pull Requests are closed, delete the environment.'),
  );

  // Pull Request Environment method.
  $form['project']['settings']['github']['pull_request_environments_method'] = array(
    '#type' => 'radios',
    '#title' => t('Pull Request Environment Deploy Method'),
    '#default_value' => $node->project->settings->github['pull_request_environments_method'],
    '#description' => t('Select the method for creating the pull request environments.'),
    '#options' => array(
      'install' => t('Run the install profile %profile', array('%profile' =>  $project->install_profile)),
      'clone_live' => t('Clone the live environment: %live', array('%live' => $project->settings->live['live_environment'])),
    )
  );
  return $form;
}

/**
 * Implementation of hook_validate().
 *
 * This function is no longer used since we have a ctools wizard for
 * project creation.
 */
function devshop_projects_validate($node, &$form) {

  // @TODO: if there is no "live domain url" then unset the values for environment aliases

}
