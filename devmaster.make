;
; Builds the DevShop front end: DevMaster
;
; This makefile is included remotely by the devshop installer.
;
core = 7.x
api = 2

projects[drupal][type] = "core"

defaults[projects][subdir] = "contrib"
defaults[projects][type] = "module"

; Aegir Modules
projects[hosting][subdir] = aegir
projects[hosting][version] = 3.x

projects[hosting_git][subdir] = aegir
projects[hosting_git][version] = 3.x

projects[hosting_remote_import][subdir] = aegir
projects[hosting_remote_import][version] = 3.x

projects[hosting_site_backup_manager][subdir] = aegir
projects[hosting_site_backup_manager][version] = 3.x

projects[hosting_tasks_extra][subdir] = aegir
projects[hosting_tasks_extra][version] = 3.x

projects[hosting_filemanager][subdir] = aegir
projects[hosting_filemanager][version] = 1.x

;projects[hosting_solr][subdir] = aegir
;projects[hosting_solr][version] = 1.x

;projects[hosting_logs][subdir] = aegir
;projects[hosting_logs][version] = 1.x

; Contrib
projects[admin_menu][version] = "3.0-rc5"
projects[betterlogin][version] = 1.2
projects[bootstrap_tour][version] = 1.0-beta8
projects[ctools][version] = "1.6"
projects[entity][version] = 1.5
projects[openidadmin][version] = "1.0"
projects[overlay_paths][version] = 1.3
projects[views][version] = "3.8"
projects[views_bulk_operations][version] = "3.2"
projects[jquery_update][version] = "2.5"

; DevShop modules
; @TODO: Move devshop_hosting these into devmaster
projects[devshop_hosting][subdir] = devshop
projects[devshop_hosting][download][type] = git
projects[devshop_hosting][download][branch] = 7.x-2.x

; Modules - Temporary

; Add devel while we're in the 7.x-3.x dev cycle;
; enabled in hostmaster.info.
projects[devel][subdir] = developer
projects[devel_debug_log][subdir] = developer