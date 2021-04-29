# Destination directory. Use "test/" for testing
destinationDir = ""

# Put the pages
pagesList = ["about.html", "research.html" ]

# Open the index.html file
indexfile = "index.html"
header = ""

with open(indexfile, 'r') as input :
    for line in input:
        if 'ABOVE THIS LINE'  in line :
            break
        header += line
        
# Add header to web content
for page in pagesList :
    # Make the menu entry blue
    newHeader = header.replace('<a class="current"', '<a')
    newHeader = newHeader.replace('<a href="'+page+'">',
                                  '<a class="current" href="'+page+'">')

    # Read page content
    with open(page, 'r') as input :
        startReading = False
        pageContent = ""
        for line in input:
            if 'ABOVE THIS LINE'  in line :
                startReading = True
            if startReading : pageContent+=line

    # Write the new file
    newfileName = destinationDir + page
    print("Creating file "+ newfileName)
    with open(newfileName, "w") as writer :
        writer.write( newHeader )
        writer.write( pageContent )
