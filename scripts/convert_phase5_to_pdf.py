#!/usr/bin/env python3
"""
Phase 5 Documentation - Convert Markdown to PDF
Python script (cross-platform)
Requires: markdown, weasyprint or pdfkit
Install: pip install markdown weasyprint
"""

import os
import sys
import subprocess
from pathlib import Path
from datetime import datetime

# ANSI color codes for terminal output
class Colors:
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    GRAY = '\033[90m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def print_header(text):
    print(f"\n{Colors.CYAN}{Colors.BOLD}{'='*60}{Colors.RESET}")
    print(f"{Colors.CYAN}{Colors.BOLD}{text}{Colors.RESET}")
    print(f"{Colors.CYAN}{Colors.BOLD}{'='*60}{Colors.RESET}\n")

def print_success(text):
    print(f"{Colors.GREEN}✓ {text}{Colors.RESET}")

def print_error(text):
    print(f"{Colors.RED}✗ {text}{Colors.RESET}")

def print_info(text):
    print(f"{Colors.YELLOW}{text}{Colors.RESET}")

def print_gray(text):
    print(f"{Colors.GRAY}{text}{Colors.RESET}")

def check_pandoc():
    """Check if pandoc is installed"""
    try:
        result = subprocess.run(['pandoc', '--version'], 
                              capture_output=True, 
                              text=True, 
                              check=True)
        version = result.stdout.split('\n')[0]
        print_success(f"pandoc found: {version}")
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print_error("pandoc is not installed!")
        print()
        print_info("Install pandoc using one of these methods:")
        print_info("  Windows: choco install pandoc")
        print_info("  macOS:   brew install pandoc")
        print_info("  Linux:   sudo apt install pandoc")
        print_info("  Manual:  https://pandoc.org/installing.html")
        print()
        return False

def convert_md_to_pdf(input_file, output_file, title, author, date):
    """Convert Markdown to PDF using pandoc"""
    
    pandoc_args = [
        'pandoc',
        input_file,
        '-o', output_file,
        '--pdf-engine=xelatex',
        '--variable', 'geometry:margin=1in',
        '--variable', 'fontsize=11pt',
        '--variable', 'colorlinks=true',
        '--variable', 'linkcolor=blue',
        '--variable', 'urlcolor=blue',
        '--variable', 'toccolor=black',
        '--toc',
        '--toc-depth=3',
        '--number-sections',
        '--highlight-style=tango',
        '--metadata', f'title={title}',
        '--metadata', f'author={author}',
        '--metadata', f'date={date}'
    ]
    
    try:
        subprocess.run(pandoc_args, check=True, capture_output=True)
        return True
    except subprocess.CalledProcessError as e:
        print_error(f"Conversion failed: {e.stderr.decode()}")
        return False

def main():
    print_header("Phase 5 Documentation - MD to PDF Converter")
    
    # Check if pandoc is installed
    if not check_pandoc():
        sys.exit(1)
    
    print()
    
    # Create output directory
    output_dir = Path('docs/phase5_pdf')
    output_dir.mkdir(parents=True, exist_ok=True)
    print_success(f"Output directory: {output_dir}")
    print()
    
    # Define documents to convert
    documents = [
        {
            'name': 'PHASE5_EXECUTIVE_SUMMARY.md',
            'title': 'Phase 5: USB Mode Switching - Executive Summary',
            'output': 'Phase5_Executive_Summary.pdf'
        },
        {
            'name': 'PHASE5_USB_MODE_SWITCHING_RECOMMENDATION.md',
            'title': 'Phase 5: USB Mode Switching - Technical Recommendation',
            'output': 'Phase5_Technical_Recommendation.pdf'
        },
        {
            'name': 'PHASE5_IMPLEMENTATION_CHECKLIST.md',
            'title': 'Phase 5: USB Mode Switching - Implementation Checklist',
            'output': 'Phase5_Implementation_Checklist.pdf'
        },
        {
            'name': 'PHASE5_REVIEW_RESPONSE.md',
            'title': 'Phase 5: USB Mode Switching - Review Response',
            'output': 'Phase5_Review_Response.pdf'
        }
    ]
    
    # Metadata
    author = "Augment Agent (AI Engineering Assistant)"
    date = datetime.now().strftime("%B %d, %Y")
    
    # Convert each document
    success_count = 0
    fail_count = 0
    
    for doc in documents:
        input_file = Path(doc['name'])
        output_file = output_dir / doc['output']
        
        print(f"{Colors.CYAN}Converting: {input_file}{Colors.RESET}")
        print_gray(f"  → {output_file}")
        
        if not input_file.exists():
            print_error(f"  Input file not found!")
            fail_count += 1
            print()
            continue
        
        if convert_md_to_pdf(str(input_file), str(output_file), 
                            doc['title'], author, date):
            file_size = output_file.stat().st_size / 1024  # KB
            print_success(f"  Success! ({file_size:.1f} KB)")
            success_count += 1
        else:
            fail_count += 1
        
        print()
    
    # Summary
    print_header("Conversion Summary")
    print_success(f"Successful: {success_count}")
    if fail_count > 0:
        print_error(f"Failed: {fail_count}")
    print()
    print_info(f"Output directory: {output_dir}")
    print()
    
    # Open output directory (platform-specific)
    if success_count > 0:
        print_info("Opening output directory...")
        if sys.platform == 'win32':
            os.startfile(output_dir)
        elif sys.platform == 'darwin':
            subprocess.run(['open', output_dir])
        else:
            subprocess.run(['xdg-open', output_dir])
    
    print_success("Done!")

if __name__ == '__main__':
    main()

