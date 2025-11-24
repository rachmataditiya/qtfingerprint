#!/bin/bash

# Setup script for fingerprint database
# Creates user, database, and runs migrations

set -e

DB_USER="finger"
DB_PASSWORD="finger"
DB_NAME="fingerprint_db"
DB_HOST="localhost"
DB_PORT="5432"

echo "🔧 Setting up PostgreSQL database for Fingerprint Backend..."
echo ""

# Check if PostgreSQL is running
if ! pg_isready -h $DB_HOST -p $DB_PORT > /dev/null 2>&1; then
    echo "❌ PostgreSQL is not running on $DB_HOST:$DB_PORT"
    echo "   Please start PostgreSQL and try again"
    exit 1
fi

echo "✅ PostgreSQL is running"

# Try to find admin user (postgres, current user, or first superuser)
ADMIN_USER=""
for user in postgres $(whoami); do
    if psql -h $DB_HOST -p $DB_PORT -U $user -c "SELECT 1;" > /dev/null 2>&1; then
        ADMIN_USER=$user
        break
    fi
done

if [ -z "$ADMIN_USER" ]; then
    echo "❌ Cannot find PostgreSQL admin user"
    echo ""
    echo "Please run manually as PostgreSQL superuser:"
    echo "  psql -U <your_admin_user> -c \"CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';\""
    echo "  psql -U <your_admin_user> -c \"CREATE DATABASE $DB_NAME OWNER $DB_USER;\""
    echo ""
    echo "Or if user already exists, just create database:"
    echo "  psql -U <your_admin_user> -c \"CREATE DATABASE $DB_NAME OWNER $DB_USER;\""
    exit 1
fi

echo "📝 Using admin user: $ADMIN_USER"

# Check if user exists
if psql -h $DB_HOST -p $DB_PORT -U $ADMIN_USER -tAc "SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'" 2>/dev/null | grep -q 1; then
    echo "✅ User '$DB_USER' already exists"
else
    echo "📝 Creating user '$DB_USER'..."
    if psql -h $DB_HOST -p $DB_PORT -U $ADMIN_USER -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';" 2>/dev/null; then
        echo "✅ User '$DB_USER' created"
    else
        echo "⚠️  Failed to create user (might need different admin user)"
        echo "   Please create manually: CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';"
    fi
fi

# Check if database exists
if psql -h $DB_HOST -p $DB_PORT -U $ADMIN_USER -lqt 2>/dev/null | cut -d \| -f 1 | grep -qw $DB_NAME; then
    echo "✅ Database '$DB_NAME' already exists"
else
    echo "📝 Creating database '$DB_NAME'..."
    if psql -h $DB_HOST -p $DB_PORT -U $ADMIN_USER -c "CREATE DATABASE $DB_NAME OWNER $DB_USER;" 2>/dev/null; then
        echo "✅ Database '$DB_NAME' created"
    else
        echo "❌ Failed to create database"
        echo "   Please create manually: CREATE DATABASE $DB_NAME OWNER $DB_USER;"
        exit 1
    fi
fi

# Grant privileges
echo "📝 Granting privileges..."
psql -h $DB_HOST -p $DB_PORT -U $ADMIN_USER -d $DB_NAME -c "GRANT ALL PRIVILEGES ON DATABASE $DB_NAME TO $DB_USER;" 2>/dev/null || {
    echo "⚠️  Failed to grant privileges (might already be granted)"
}

# Test connection
echo "🔍 Testing connection..."
if PGPASSWORD=$DB_PASSWORD psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "SELECT 1;" > /dev/null 2>&1; then
    echo "✅ Connection test successful"
else
    echo "⚠️  Connection test failed (might need to set PGPASSWORD)"
    echo "   Testing with password prompt..."
    if psql -h $DB_HOST -p $DB_PORT -U $DB_USER -d $DB_NAME -c "SELECT 1;" > /dev/null 2>&1; then
        echo "✅ Connection test successful"
    else
        echo "❌ Connection test failed"
        echo "   Please check your PostgreSQL configuration"
        exit 1
    fi
fi

echo ""
echo "✅ Database setup complete!"
echo ""
echo "📋 Connection details:"
echo "   Host: $DB_HOST"
echo "   Port: $DB_PORT"
echo "   User: $DB_USER"
echo "   Database: $DB_NAME"
echo "   URL: postgresql://$DB_USER:$DB_PASSWORD@$DB_HOST:$DB_PORT/$DB_NAME"
echo ""
echo "🚀 You can now run the backend with:"
echo "   cargo run"
echo ""

