# SIS3315 Implementation

## First, make sure you have configured an SSH key beforehand (ssh-keygen)

## Setup

```bash
git clone git@github.com:aloisedijoux/SIS3315_implementation.git
cd SIS3315_implementation
git checkout -b your-name
git push -u origin feature/your-name
```

## Workflow

```bash
git add .
git commit -m "feat: ..."
git push

git fetch origin
git rebase origin/main  # synchronize with main
```
merges on `main` are done via Pull Request only.
