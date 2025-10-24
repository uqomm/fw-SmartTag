#!/usr/bin/env python3
"""
JIRA Project Manager - Unified script for FW-Gateway JIRA operations
Consolidates all JIRA management functionality into a single tool with parameters

Usage:
    python jira_manager.py --action <action> [options]

Actions:
    create-version      Create a new version in JIRA
    create-task         Create a new task
    update-task         Update an existing task (summary, description, assignee, labels)
    create-subtasks     Create subtasks under a parent task
    add-worklog         Add worklog entries
    delete-worklog      Delete worklog entries
    mark-done           Mark tasks as done
    assign-task         Assign task to user
    set-dates           Set start/end dates for task
    link-tasks          Create relationships between tasks
    list-tasks          List all tasks in project
    get-details         Get detailed task information
    update-from-changelog Update JIRA from CHANGELOG.md

Examples:
    python jira_manager.py --action create-version --version v2.6.0
    python jira_manager.py --action mark-done --task FG-4
    python jira_manager.py --action add-worklog --task FG-4 --hours 4 --comment "FSK implementation"
    python jira_manager.py --action list-tasks
"""

import os
import sys
import argparse
import re
from datetime import datetime, timedelta
from pathlib import Path
from typing import List, Dict, Optional
from dotenv import load_dotenv
from jira import JIRA

# ============================================================================
# CONFIGURATION & SETUP
# ============================================================================

class Colors:
    """Console color codes"""
    RESET = '\033[0m'
    RED = '\033[31m'
    GREEN = '\033[32m'
    YELLOW = '\033[33m'
    BLUE = '\033[34m'
    MAGENTA = '\033[35m'
    CYAN = '\033[36m'
    WHITE = '\033[37m'
    BOLD = '\033[1m'

def color_text(text: str, color: str) -> str:
    """Apply color to text"""
    return f"{color}{text}{Colors.RESET}"

def print_header(text: str):
    """Print formatted header"""
    print("\n" + "=" * 70)
    print(color_text(text, Colors.CYAN + Colors.BOLD))
    print("=" * 70)

def print_success(text: str):
    """Print success message"""
    print(color_text(f"✅ {text}", Colors.GREEN))

def print_error(text: str):
    """Print error message"""
    print(color_text(f"❌ {text}", Colors.RED))

def print_warning(text: str):
    """Print warning message"""
    print(color_text(f"⚠️  {text}", Colors.YELLOW))

def print_info(text: str):
    """Print info message"""
    print(color_text(f"ℹ️  {text}", Colors.BLUE))

# Load environment variables
load_dotenv()  # Load from .env file

JIRA_URL = os.getenv("JIRA_URL")
JIRA_EMAIL = os.getenv("JIRA_EMAIL")
JIRA_API_TOKEN = os.getenv("JIRA_API_TOKEN")
PROJECT_KEY = os.getenv("JIRA_PROJECT_KEY")

# Validate environment
if not all([JIRA_URL, JIRA_EMAIL, JIRA_API_TOKEN, PROJECT_KEY]):
    print_error("Missing JIRA environment variables. Check .env file")
    print_info("Copy .env.example to .env and fill in your credentials")
    sys.exit(1)

# ============================================================================
# JIRA CONNECTION
# ============================================================================

class JiraManager:
    """Unified JIRA project manager"""
    
    def __init__(self):
        """Initialize JIRA connection"""
        try:
            self.jira = JIRA(
                server=JIRA_URL,
                basic_auth=(JIRA_EMAIL, JIRA_API_TOKEN)
            )
            self.project_key = PROJECT_KEY
            print_success(f"Connected to JIRA: {JIRA_URL}")
            print_info(f"Project: {PROJECT_KEY}")
        except Exception as e:
            print_error(f"Failed to connect to JIRA: {e}")
            sys.exit(1)
    
    # ========================================================================
    # VERSION MANAGEMENT
    # ========================================================================
    
    def create_version(self, version_name: str, description: str = None, 
                      release_date: str = None, released: bool = False) -> Dict:
        """Create a new version in JIRA"""
        print_header(f"CREATING VERSION: {version_name}")
        
        try:
            # Check if version already exists
            existing_versions = self.jira.project_versions(self.project_key)
            for v in existing_versions:
                if v.name == version_name:
                    print_warning(f"Version {version_name} already exists")
                    return {'status': 'exists', 'version': v}
            
            # Create version
            version_data = {
                'name': version_name,
                'project': self.project_key,
                'released': released
            }
            
            if description:
                version_data['description'] = description
            
            if release_date:
                version_data['releaseDate'] = release_date
            
            version = self.jira.create_version(**version_data)
            
            print_success(f"Version created: {version.name}")
            print_info(f"ID: {version.id}")
            print_info(f"URL: {JIRA_URL}/projects/{self.project_key}/versions/{version.id}")
            
            return {'status': 'created', 'version': version}
            
        except Exception as e:
            print_error(f"Failed to create version: {e}")
            return {'status': 'error', 'error': str(e)}
    
    # ========================================================================
    # TASK MANAGEMENT
    # ========================================================================
    
    def create_task(self, summary: str, description: str = None, 
                   issue_type: str = 'Task', parent_key: str = None,
                   labels: List[str] = None) -> Dict:
        """Create a new task or subtask"""
        print_header(f"CREATING {issue_type.upper()}: {summary}")
        
        try:
            fields = {
                'project': {'key': self.project_key},
                'summary': summary,
                'issuetype': {'name': issue_type}
            }
            
            if description:
                fields['description'] = description
            
            if parent_key:
                fields['parent'] = {'key': parent_key}
            
            if labels:
                fields['labels'] = labels
            
            issue = self.jira.create_issue(fields=fields)
            
            print_success(f"Created: {issue.key} - {summary}")
            return {'status': 'created', 'issue': issue}
            
        except Exception as e:
            print_error(f"Failed to create task: {e}")
            return {'status': 'error', 'error': str(e)}
    
    def create_subtasks(self, parent_key: str, subtasks: List[Dict]) -> List[Dict]:
        """Create multiple subtasks under a parent"""
        print_header(f"CREATING SUBTASKS UNDER: {parent_key}")
        
        results = []
        for subtask_data in subtasks:
            result = self.create_task(
                summary=subtask_data.get('summary'),
                description=subtask_data.get('description'),
                issue_type='Subtarea',
                parent_key=parent_key,
                labels=subtask_data.get('labels')
            )
            results.append(result)
        
        return results
    
    def mark_done(self, task_key: str, comment: str = None) -> Dict:
        """Mark a task as done"""
        print_header(f"MARKING AS DONE: {task_key}")
        
        try:
            issue = self.jira.issue(task_key)
            print_info(f"Current status: {issue.fields.status.name}")
            
            # Find completion transition
            transitions = self.jira.transitions(issue)
            done_transition = None
            
            for t in transitions:
                if any(keyword in t['name'].lower() 
                      for keyword in ['done', 'cerrad', 'hech', 'listo', 'complet']):
                    done_transition = t['id']
                    print_info(f"Found transition: {t['name']} (ID: {t['id']})")
                    break
            
            if not done_transition:
                print_warning("No completion transition found. Available:")
                for t in transitions:
                    print(f"  - {t['name']} (ID: {t['id']})")
                return {'status': 'no_transition', 'transitions': transitions}
            
            # Transition to done
            self.jira.transition_issue(issue, done_transition)
            print_success(f"Transitioned to: Done")
            
            # Add comment if provided
            if comment:
                self.jira.add_comment(task_key, comment)
                print_success("Comment added")
            
            return {'status': 'done', 'issue': issue}
            
        except Exception as e:
            print_error(f"Failed to mark as done: {e}")
            return {'status': 'error', 'error': str(e)}
    
    # ========================================================================
    # WORKLOG MANAGEMENT
    # ========================================================================
    
    def add_worklog(self, task_key: str, hours: float, 
                   comment: str = None, date: str = None) -> Dict:
        """Add worklog entry"""
        print_header(f"ADDING WORKLOG: {task_key}")
        
        try:
            # Convert hours to seconds for accurate logging
            time_spent_seconds = int(hours * 3600)
            
            worklog_data = {
                'issue': task_key,
                'timeSpentSeconds': time_spent_seconds
            }
            
            if comment:
                worklog_data['comment'] = comment
            
            if date:
                worklog_data['started'] = datetime.strptime(date, '%Y-%m-%d')
            
            worklog = self.jira.add_worklog(**worklog_data)
            
            print_success(f"Worklog added: {hours}h")
            if comment:
                print_info(f"Comment: {comment}")
            
            return {'status': 'added', 'worklog': worklog}
            
        except Exception as e:
            print_error(f"Failed to add worklog: {e}")
            return {'status': 'error', 'error': str(e)}
    
    def delete_worklog(self, task_key: str, worklog_id: str) -> Dict:
        """Delete a worklog entry"""
        print_header(f"DELETING WORKLOG: {worklog_id} from {task_key}")
        
        try:
            # Delete the worklog
            self.jira.worklog(task_key, worklog_id).delete()
            
            print_success(f"Worklog {worklog_id} deleted from {task_key}")
            return {'status': 'success', 'task': task_key, 'worklog_id': worklog_id}
            
        except Exception as e:
            print_error(f"Failed to delete worklog: {e}")
            return {'status': 'error', 'error': str(e)}
    
    # ========================================================================
    # TASK RELATIONSHIPS
    # ========================================================================
    
    def link_tasks(self, inward_key: str, outward_key: str, 
                  link_type: str = "Relates", comment: str = None) -> Dict:
        """Create link between tasks"""
        print_header(f"LINKING: {inward_key} → {outward_key}")
        
        try:
            link_data = {
                'type': link_type,
                'inwardIssue': inward_key,
                'outwardIssue': outward_key
            }
            
            if comment:
                link_data['comment'] = {'body': comment}
            
            self.jira.create_issue_link(**link_data)
            
            print_success(f"Linked: {link_type}")
            return {'status': 'linked'}
            
        except Exception as e:
            print_error(f"Failed to link tasks: {e}")
            return {'status': 'error', 'error': str(e)}
    
    # ========================================================================
    # QUERYING & REPORTING
    # ========================================================================
    
    def list_tasks(self, status: str = None, issue_type: str = None, 
                  max_results: int = 50) -> List[Dict]:
        """List tasks in project"""
        print_header("LISTING TASKS")
        
        try:
            jql = f'project={self.project_key}'
            
            if status:
                jql += f' AND status="{status}"'
            
            if issue_type:
                jql += f' AND issuetype="{issue_type}"'
            
            jql += ' ORDER BY created DESC'
            
            issues = self.jira.search_issues(jql, maxResults=max_results)
            
            print_info(f"Found {len(issues)} tasks:\n")
            
            tasks = []
            for issue in issues:
                task_info = {
                    'key': issue.key,
                    'type': issue.fields.issuetype.name,
                    'summary': issue.fields.summary,
                    'status': issue.fields.status.name
                }
                tasks.append(task_info)
                
                # Print formatted
                type_color = Colors.CYAN if issue.fields.issuetype.name == 'Epic' else Colors.BLUE
                status_color = Colors.GREEN if 'done' in issue.fields.status.name.lower() else Colors.YELLOW
                
                print(f"{color_text(issue.key, type_color)}: "
                      f"{color_text(issue.fields.issuetype.name, type_color)} - "
                      f"{issue.fields.summary} "
                      f"[{color_text(issue.fields.status.name, status_color)}]")
            
            return tasks
            
        except Exception as e:
            print_error(f"Failed to list tasks: {e}")
            return []
    
    def get_task_details(self, task_key: str) -> Dict:
        """Get detailed information about a task"""
        print_header(f"TASK DETAILS: {task_key}")
        
        try:
            issue = self.jira.issue(task_key)
            
            details = {
                'key': issue.key,
                'summary': issue.fields.summary,
                'type': issue.fields.issuetype.name,
                'status': issue.fields.status.name,
                'created': issue.fields.created,
                'updated': issue.fields.updated
            }
            
            print(f"\n{color_text('Key:', Colors.BOLD)} {issue.key}")
            print(f"{color_text('Type:', Colors.BOLD)} {issue.fields.issuetype.name}")
            print(f"{color_text('Summary:', Colors.BOLD)} {issue.fields.summary}")
            print(f"{color_text('Status:', Colors.BOLD)} {issue.fields.status.name}")
            print(f"{color_text('Created:', Colors.BOLD)} {issue.fields.created}")
            
            # Show assignee
            if hasattr(issue.fields, 'assignee') and issue.fields.assignee:
                print(f"{color_text('Assignee:', Colors.BOLD)} {issue.fields.assignee.displayName} ({issue.fields.assignee.emailAddress})")
            else:
                print(f"{color_text('Assignee:', Colors.BOLD)} Unassigned")
            
            # Show dates
            if hasattr(issue.fields, 'customfield_10015') and issue.fields.customfield_10015:
                print(f"{color_text('Start Date:', Colors.BOLD)} {issue.fields.customfield_10015}")
            if hasattr(issue.fields, 'duedate') and issue.fields.duedate:
                print(f"{color_text('Due Date:', Colors.BOLD)} {issue.fields.duedate}")
            
            # Show worklogs
            try:
                worklogs = self.jira.worklogs(issue.key)
                if worklogs:
                    print(f"\n{color_text('Worklogs:', Colors.BOLD)}")
                    total_hours = 0
                    for worklog in worklogs:
                        hours = worklog.timeSpentSeconds / 3600
                        total_hours += hours
                        author = worklog.author.displayName if worklog.author else 'Unknown'
                        date = worklog.started[:10]  # YYYY-MM-DD
                        print(f"  - ID: {worklog.id} | {hours:.1f}h by {author} on {date}")
                        if worklog.comment:
                            print(f"    Comment: {worklog.comment[:100]}{'...' if len(worklog.comment) > 100 else ''}")
                    print(f"  {color_text('Total:', Colors.BOLD)} {total_hours:.1f} hours")
                else:
                    print(f"\n{color_text('Worklogs:', Colors.BOLD)} None")
            except Exception as e:
                print(f"\n{color_text('Worklogs:', Colors.BOLD)} Error loading worklogs: {e}")
            
            if issue.fields.description:
                print(f"\n{color_text('Description:', Colors.BOLD)}")
                print(issue.fields.description[:200] + "..." if len(issue.fields.description) > 200 else issue.fields.description)
            
            # Get transitions
            transitions = self.jira.transitions(issue)
            if transitions:
                print(f"\n{color_text('Available Transitions:', Colors.BOLD)}")
                for t in transitions:
                    print(f"  - {t['name']} (ID: {t['id']})")
            
            return details
            
        except Exception as e:
            print_error(f"Failed to get task details: {e}")
            return {}
    
    # ========================================================================
    # TASK ASSIGNMENT AND DATES
    # ========================================================================
    
    def assign_task(self, task_key: str, assignee: str) -> Dict:
        """Assign task to user"""
        print_header(f"ASSIGNING: {task_key} → {assignee}")
        
        try:
            issue = self.jira.issue(task_key)
            
            # Handle common assignee formats and map to accountId
            if assignee.lower() in ['arturo', 'arturo vera', 'arturo vera solivos', 'arturo@uqomm.com']:
                assignee_data = {'accountId': '61a0e909744c4d0069d53d6d'}
                display_name = 'Arturo Armando Veras Olivos'
            else:
                # For other users, try to find by accountId or name
                try:
                    # Try accountId format first
                    assignee_data = {'accountId': assignee}
                    display_name = assignee
                except:
                    # Fallback to name format
                    assignee_data = {'name': assignee}
                    display_name = assignee
            
            issue.update(assignee=assignee_data)
            
            print_success(f"Task {task_key} assigned to {display_name}")
            return {'status': 'success', 'task': task_key, 'assignee': display_name}
            
        except Exception as e:
            print_error(f"Failed to assign task: {e}")
            return {'status': 'error', 'error': str(e)}
    
    def set_dates(self, task_key: str, start_date: str = None, end_date: str = None) -> Dict:
        """Set start and end dates for task"""
        print_header(f"SETTING DATES: {task_key}")
        
        try:
            issue = self.jira.issue(task_key)
            
            # Prepare update data
            update_data = {}
            
            if start_date:
                # Use standard start date field
                from datetime import datetime
                start_datetime = datetime.strptime(start_date, '%Y-%m-%d')
                update_data['customfield_10015'] = start_datetime.strftime('%Y-%m-%d')  # Start date
                print_info(f"Start date: {start_date}")
                
            if end_date:
                # Use standard due date field
                from datetime import datetime
                due_datetime = datetime.strptime(end_date, '%Y-%m-%d')
                update_data['duedate'] = due_datetime.strftime('%Y-%m-%d')  # Due date
                print_info(f"End date: {end_date}")
            
            if update_data:
                issue.update(**update_data)
                print_success(f"Dates updated for {task_key}")
            else:
                print_info("No dates to update")
            
            return {'status': 'success', 'task': task_key, 'start_date': start_date, 'end_date': end_date}
            
        except Exception as e:
            print_error(f"Failed to set dates: {e}")
            return {'status': 'error', 'error': str(e)}
    
    # ========================================================================
    # CHANGELOG INTEGRATION
    # ========================================================================
    
    def update_from_changelog(self, changelog_path: str = "CHANGELOG.md") -> Dict:
        """Update JIRA tasks based on CHANGELOG.md"""
        print_header("UPDATING FROM CHANGELOG")
        
        try:
            # Handle multiple changelogs for sniffer-tag project
            changelog_paths = [
                "sniffer/CHANGELOG.md",
                "Persona/CHANGELOG.md"
            ]
            
            all_updates = []
            
            for path in changelog_paths:
                changelog = Path(path)
                if not changelog.exists():
                    print_info(f"Changelog not found: {path}")
                    continue
                
                print_info(f"Processing {path}...")
                content = changelog.read_text(encoding='utf-8')
                
                # Parse changelog and create/update tasks
                updates = self._process_changelog_content(content, path)
                all_updates.extend(updates)
            
            if not all_updates:
                print_info("No updates found in changelogs")
                return {'status': 'no_updates'}
            
            print_success(f"Processed {len(all_updates)} changelog entries")
            return {'status': 'updated', 'updates': all_updates}
            
        except Exception as e:
            print_error(f"Failed to update from changelog: {e}")
            return {'status': 'error', 'error': str(e)}
    
    def _process_changelog_content(self, content: str, source_path: str) -> List[Dict]:
        """Process changelog content and create/update JIRA tasks"""
        updates = []
        
        # Split by version sections
        import re
        version_pattern = r'## \[([^\]]+)\]'
        versions = re.findall(version_pattern, content)
        
        for version in versions:
            if version.lower() in ['unreleased']:
                print_info(f"Processing unreleased changes from {source_path}")
                # Create tasks for unreleased changes
                unreleased_tasks = self._extract_unreleased_tasks(content, source_path)
                updates.extend(unreleased_tasks)
            else:
                print_info(f"Processing version {version} from {source_path}")
                # Mark tasks as done for released versions
                version_tasks = self._extract_version_tasks(content, version, source_path)
                updates.extend(version_tasks)
        
        return updates
    
    def _extract_unreleased_tasks(self, content: str, source_path: str) -> List[Dict]:
        """Extract tasks from unreleased section"""
        tasks = []
        
        # Find unreleased section
        unreleased_match = re.search(r'## \[Unreleased\](.*?)(?=## \[|$)', content, re.DOTALL)
        if not unreleased_match:
            return tasks
        
        unreleased_content = unreleased_match.group(1)
        
        # Extract changes by type
        change_types = ['Added', 'Changed', 'Fixed', 'Removed']
        
        for change_type in change_types:
            pattern = rf'### {change_type}(.*?)(?=###|$)'
            match = re.search(pattern, unreleased_content, re.DOTALL | re.IGNORECASE)
            if match:
                changes = match.group(1).strip()
                # Create task for this change type
                summary = f"[{change_type}] {source_path.replace('CHANGELOG.md', '').strip('/')}"
                description = f"{change_type} changes from {source_path}:\n{changes}"
                
                task_result = self.create_task(
                    summary=summary,
                    description=description,
                    issue_type='Task',
                    labels=['changelog', 'unreleased', source_path.split('/')[0]]
                )
                tasks.append(task_result)
        
        return tasks
    
    def _extract_version_tasks(self, content: str, version: str, source_path: str) -> List[Dict]:
        """Extract and mark done tasks for released versions"""
        tasks = []
        
        # For now, just log the version - can be enhanced to mark specific tasks as done
        print_info(f"Found released version {version} in {source_path}")
        
        # Could add logic here to mark related tasks as done
        # For example, search for task keys in commit messages and mark them done
        
        return tasks
    
    def update_task(self, task_key: str, summary: str = None, description: str = None,
                   assignee: str = None, labels: List[str] = None) -> Dict:
        """Update an existing task"""
        print_header(f"UPDATING TASK: {task_key}")
        
        try:
            issue = self.jira.issue(task_key)
            
            update_data = {}
            
            if summary:
                update_data['summary'] = summary
                print_info(f"New summary: {summary}")
            
            if description:
                update_data['description'] = description
                print_info("Description updated")
            
            if assignee:
                # Handle assignee mapping
                if assignee.lower() in ['arturo', 'arturo vera', 'arturo vera solivos', 'arturo@uqomm.com']:
                    update_data['assignee'] = {'accountId': '61a0e909744c4d0069d53d6d'}
                    display_name = 'Arturo Armando Veras Olivos'
                else:
                    try:
                        update_data['assignee'] = {'accountId': assignee}
                        display_name = assignee
                    except:
                        update_data['assignee'] = {'name': assignee}
                        display_name = assignee
                print_info(f"New assignee: {display_name}")
            
            if labels is not None:
                update_data['labels'] = labels
                print_info(f"Labels: {labels}")
            
            if update_data:
                issue.update(**update_data)
                print_success(f"Task {task_key} updated successfully")
            else:
                print_info("No updates to apply")
            
            return {'status': 'updated', 'task': task_key, 'updates': update_data}
            
        except Exception as e:
            print_error(f"Failed to update task: {e}")
            return {'status': 'error', 'error': str(e)}

# ============================================================================
# COMMAND LINE INTERFACE
# ============================================================================

def main():
    """Main CLI entry point"""
    parser = argparse.ArgumentParser(
        description='JIRA Project Manager - Unified FW-Gateway JIRA operations',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  %(prog)s --action list-tasks
  %(prog)s --action create-version --version v2.6.0 --description "New features"
  %(prog)s --action create-task --summary "New task" --description "Task description"
  %(prog)s --action update-task --task FG-4 --description "Updated description"
  %(prog)s --action mark-done --task FG-4 --comment "Completed successfully"
  %(prog)s --action add-worklog --task FG-4 --hours 4 --comment "FSK development"
  %(prog)s --action delete-worklog --task FG-4 --worklog-id 12345
  %(prog)s --action assign-task --task FG-4 --assignee "username"
  %(prog)s --action set-dates --task FG-4 --start-date 2025-10-16 --end-date 2025-10-16
  %(prog)s --action create-subtasks --parent FG-5 --subtasks-json subtasks.json
  %(prog)s --action link-tasks --inward FG-4 --outward FG-3 --link-type Relates
  %(prog)s --action get-details --task FG-4
  %(prog)s --action update-from-changelog
        '''
    )
    
    # Main action
    parser.add_argument('--action', required=True,
                       choices=['create-version', 'create-task', 'update-task', 'create-subtasks',
                               'add-worklog', 'delete-worklog', 'mark-done', 'assign-task', 'set-dates',
                               'link-tasks', 'list-tasks', 'get-details', 'update-from-changelog'],
                       help='Action to perform')
    
    # Version management
    parser.add_argument('--version', help='Version name (e.g., v2.6.0)')
    parser.add_argument('--description', help='Version/task description')
    parser.add_argument('--release-date', help='Release date (YYYY-MM-DD)')
    parser.add_argument('--released', action='store_true', help='Mark version as released')
    
    # Task management
    parser.add_argument('--task', help='Task key (e.g., FG-4)')
    parser.add_argument('--summary', help='Task summary')
    parser.add_argument('--parent', help='Parent task key for subtasks')
    parser.add_argument('--type', default='Task', help='Issue type (Task, Epic, Subtarea)')
    parser.add_argument('--labels', nargs='+', help='Task labels')
    parser.add_argument('--assignee', help='Assignee username or email')
    parser.add_argument('--start-date', help='Start date (YYYY-MM-DD)')
    parser.add_argument('--end-date', help='End date (YYYY-MM-DD)')
    parser.add_argument('--subtasks-json', help='JSON file with subtask definitions')
    
    # Worklog
    parser.add_argument('--hours', type=float, help='Hours to log')
    parser.add_argument('--comment', help='Comment for worklog/task')
    parser.add_argument('--date', help='Date for worklog (YYYY-MM-DD)')
    parser.add_argument('--worklog-id', help='Worklog ID to delete')
    
    # Task linking
    parser.add_argument('--inward', help='Inward issue key for linking')
    parser.add_argument('--outward', help='Outward issue key for linking')
    parser.add_argument('--link-type', default='Relates', help='Link type')
    
    # Querying
    parser.add_argument('--status', help='Filter by status')
    parser.add_argument('--issue-type', help='Filter by issue type')
    parser.add_argument('--max-results', type=int, default=50, help='Max results to return')
    
    # Changelog
    parser.add_argument('--changelog', default='CHANGELOG.md', help='Path to CHANGELOG.md')
    
    args = parser.parse_args()
    
    # Initialize manager
    manager = JiraManager()
    
    # Execute action
    try:
        if args.action == 'create-version':
            if not args.version:
                print_error("--version required for create-version")
                sys.exit(1)
            manager.create_version(
                version_name=args.version,
                description=args.description,
                release_date=args.release_date,
                released=args.released
            )
        
        elif args.action == 'create-task':
            if not args.summary:
                print_error("--summary required for create-task")
                sys.exit(1)
            manager.create_task(
                summary=args.summary,
                description=args.description,
                issue_type=args.type,
                parent_key=args.parent,
                labels=args.labels
            )
        
        elif args.action == 'update-task':
            if not args.task:
                print_error("--task required for update-task")
                sys.exit(1)
            manager.update_task(
                task_key=args.task,
                summary=args.summary,
                description=args.description,
                assignee=args.assignee,
                labels=args.labels
            )
        
        elif args.action == 'create-subtasks':
            if not args.parent:
                print_error("--parent required for create-subtasks")
                sys.exit(1)
            if not args.subtasks_json:
                print_error("--subtasks-json required for create-subtasks")
                sys.exit(1)
            
            import json
            with open(args.subtasks_json) as f:
                subtasks = json.load(f)
            manager.create_subtasks(args.parent, subtasks)
        
        elif args.action == 'add-worklog':
            if not args.task or not args.hours:
                print_error("--task and --hours required for add-worklog")
                sys.exit(1)
            manager.add_worklog(
                task_key=args.task,
                hours=args.hours,
                comment=args.comment,
                date=args.date
            )
        
        elif args.action == 'delete-worklog':
            if not args.task or not args.worklog_id:
                print_error("--task and --worklog-id required for delete-worklog")
                sys.exit(1)
            manager.delete_worklog(args.task, args.worklog_id)
        
        elif args.action == 'mark-done':
            if not args.task:
                print_error("--task required for mark-done")
                sys.exit(1)
            manager.mark_done(args.task, args.comment)
        
        elif args.action == 'assign-task':
            if not args.task or not args.assignee:
                print_error("--task and --assignee required for assign-task")
                sys.exit(1)
            manager.assign_task(args.task, args.assignee)
        
        elif args.action == 'set-dates':
            if not args.task:
                print_error("--task required for set-dates")
                sys.exit(1)
            manager.set_dates(args.task, args.start_date, args.end_date)
        
        elif args.action == 'link-tasks':
            if not args.inward or not args.outward:
                print_error("--inward and --outward required for link-tasks")
                sys.exit(1)
            manager.link_tasks(
                inward_key=args.inward,
                outward_key=args.outward,
                link_type=args.link_type,
                comment=args.comment
            )
        
        elif args.action == 'list-tasks':
            manager.list_tasks(
                status=args.status,
                issue_type=args.issue_type,
                max_results=args.max_results
            )
        
        elif args.action == 'get-details':
            if not args.task:
                print_error("--task required for get-details")
                sys.exit(1)
            manager.get_task_details(args.task)
        
        elif args.action == 'update-from-changelog':
            manager.update_from_changelog(args.changelog)
        
        print("\n" + color_text("✅ Operation completed successfully!", Colors.GREEN + Colors.BOLD))
        
    except KeyboardInterrupt:
        print("\n" + color_text("⚠️  Operation cancelled by user", Colors.YELLOW))
        sys.exit(1)
    except Exception as e:
        print_error(f"Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()
