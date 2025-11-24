# PowerShell script for Windows
# Setup script for fingerprint database

$DB_USER = "finger"
$DB_PASSWORD = "finger"
$DB_NAME = "fingerprint_db"
$DB_HOST = "localhost"
$DB_PORT = "5432"

Write-Host "🔧 Setting up PostgreSQL database for Fingerprint Backend..." -ForegroundColor Cyan
Write-Host ""

# Check if psql is available
try {
    $null = Get-Command psql -ErrorAction Stop
} catch {
    Write-Host "❌ psql command not found. Please install PostgreSQL client tools." -ForegroundColor Red
    exit 1
}

# Check if PostgreSQL is running
try {
    $result = & pg_isready -h $DB_HOST -p $DB_PORT 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "PostgreSQL not running"
    }
    Write-Host "✅ PostgreSQL is running" -ForegroundColor Green
} catch {
    Write-Host "❌ PostgreSQL is not running on ${DB_HOST}:${DB_PORT}" -ForegroundColor Red
    Write-Host "   Please start PostgreSQL and try again" -ForegroundColor Yellow
    exit 1
}

# Check if user exists
$userExists = & psql -h $DB_HOST -p $DB_PORT -U postgres -tAc "SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'" 2>&1
if ($userExists -match "1") {
    Write-Host "✅ User '$DB_USER' already exists" -ForegroundColor Green
} else {
    Write-Host "📝 Creating user '$DB_USER'..." -ForegroundColor Yellow
    $env:PGPASSWORD = "postgres"  # Default postgres password, adjust if needed
    & psql -h $DB_HOST -p $DB_PORT -U postgres -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ User '$DB_USER' created" -ForegroundColor Green
    } else {
        Write-Host "⚠️  Failed to create user (might already exist or need different credentials)" -ForegroundColor Yellow
    }
    Remove-Item Env:\PGPASSWORD
}

# Check if database exists
$dbExists = & psql -h $DB_HOST -p $DB_PORT -U postgres -lqt 2>&1 | Select-String -Pattern $DB_NAME
if ($dbExists) {
    Write-Host "✅ Database '$DB_NAME' already exists" -ForegroundColor Green
} else {
    Write-Host "📝 Creating database '$DB_NAME'..." -ForegroundColor Yellow
    $env:PGPASSWORD = "postgres"  # Default postgres password, adjust if needed
    & psql -h $DB_HOST -p $DB_PORT -U postgres -c "CREATE DATABASE $DB_NAME OWNER $DB_USER;" 2>&1 | Out-Null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Database '$DB_NAME' created" -ForegroundColor Green
    } else {
        Write-Host "❌ Failed to create database" -ForegroundColor Red
        Remove-Item Env:\PGPASSWORD
        exit 1
    }
    Remove-Item Env:\PGPASSWORD
}

# Grant privileges
Write-Host "📝 Granting privileges..." -ForegroundColor Yellow
$env:PGPASSWORD = "postgres"  # Default postgres password, adjust if needed
& psql -h $DB_HOST -p $DB_PORT -U postgres -d $DB_NAME -c "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;" 2>&1 | Out-Null
Remove-Item Env:\PGPASSWORD

# Test connection
Write-Host "🔍 Testing connection..." -ForegroundColor Yellow
$env:PGPASSWORD = $DB_PASSWORD
$testResult = & psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "SELECT 1;" 2>&1
Remove-Item Env:\PGPASSWORD

if ($LASTEXITCODE -eq 0) {
    Write-Host "✅ Connection test successful" -ForegroundColor Green
} else {
    Write-Host "❌ Connection test failed" -ForegroundColor Red
    Write-Host "   Please check your PostgreSQL configuration" -ForegroundColor Yellow
    exit 1
}

Write-Host ""
Write-Host "✅ Database setup complete!" -ForegroundColor Green
Write-Host ""
Write-Host "📋 Connection details:" -ForegroundColor Cyan
Write-Host "   Host: $DB_HOST"
Write-Host "   Port: $DB_PORT"
Write-Host "   User: $DB_USER"
Write-Host "   Database: $DB_NAME"
Write-Host "   URL: postgresql://${DB_USER}:${DB_PASSWORD}@${DB_HOST}:${DB_PORT}/${DB_NAME}"
Write-Host ""
Write-Host "🚀 You can now run the backend with:" -ForegroundColor Green
Write-Host "   cargo run"
Write-Host ""

