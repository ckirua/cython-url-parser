from postgres_url import URL

# Parse a URL
url = URL.parse("postgresql://user:pass@localhost:5432/mydb?sslmode=require")

# Access components
print(url.host)  # "localhost"
print(url.port)  # "5432"
print(url.database)  # "mydb"
print(url.username)  # "user"
print(url.password)  # "pass"
print(url.options)  # "sslmode=require"
print(url.ssl_enabled)  # True

# Modify components
url.host = "newhost"
url.port = "5433"
url.database = "newdb"

# Generate URL string
new_url = url.to_string()
print(new_url)  # "postgresql://user:pass@newhost:5433/newdb?sslmode=require"
