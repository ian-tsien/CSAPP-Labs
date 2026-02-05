import os
import requests

# GitHub repo info
OWNER = "ian-tsien"
REPO = "CSAPP-Labs"
PATH = "malloc-lab/traces"

# Where to save downloaded files
SAVE_DIR = "traces"

# Create save directory
os.makedirs(SAVE_DIR, exist_ok=True)

# GitHub API URL for directory contents
api_url = f"https://api.github.com/repos/{OWNER}/{REPO}/contents/{PATH}"

headers = {
    "Accept": "application/vnd.github.v3+json",
    # Optional: Add a GitHub token here to increase API rate limit:
    # "Authorization": "token YOUR_GITHUB_TOKEN"
}

# Get list of files
resp = requests.get(api_url, headers=headers)
resp.raise_for_status()
files = resp.json()

for fileinfo in files:
    if fileinfo["type"] == "file":
        filename = fileinfo["name"]
        download_url = fileinfo["download_url"]

        print(f"Downloading {filename}...")

        file_resp = requests.get(download_url)
        file_resp.raise_for_status()

        with open(os.path.join(SAVE_DIR, filename), "wb") as f:
            f.write(file_resp.content)

print("Done!")
