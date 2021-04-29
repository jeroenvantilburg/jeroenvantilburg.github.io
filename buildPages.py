#import os
#if os.path.isfile(os.environ['PYTHONSTARTUP']):
#    execfile(os.environ['PYTHONSTARTUP'])
#import os.path, time

destinationDir = "test/"
pagesList = {
             "about.html"        : "img/Universe.png",
             "research.html"     : "img/ijs.jpg"
             }

#print( "tot hier" );
# Get the regular expression library
#import re
#space = re.compile(r'\s+')

# Open the index.html file
indexfile = "index.html"
header = ""

with open(indexfile, 'r') as input :
    for line in input:
        if 'ABOVE THIS LINE'  in line :
            break
        header += line

#print( header )

        
# Add header to web content
for page in pagesList :
    # replace the picture
    #header = header.replace("profile.jpg",pagesList[page])

    # Make the menu entry blue
    #header = header.replace('<a href="'+page+'.html">',
    #                        '<a class="current" href="'+page+'.html">')

    # open the page html content
    #oldfile = open(page+".html")

    # Read page content
    with open(page, 'r') as input :
        startReading = False
        pageContent = ""
        for line in input:
            if 'ABOVE THIS LINE'  in line :
                startReading = True
            if startReading : pageContent+=line
    #print( pageContent )
    

    # Write the new file
    newfileName = destinationDir + page
    print("Creating file "+ newfileName)
    with open(newfileName, "w") as writer :
        writer.write( header )
        writer.write( pageContent )

    
    #newfile = open(newfileName,"w")
    #newfile.write(header)
    #newfile.write(oldfile.read())


