import os, sys, re

# A function to convert a string to a number of base 26. The input is
# assumed to be a string of character
def char_to_num(index):
    index_upper=index.upper()
    power = len(index_upper)
    total = 0
    pos = 0
    for char in index:
        total = total + (ord(char)-ord('A')) * pow(26,power-pos-1)
        pos = pos+1
    return total

# The function to create image pairs for registration
def create_pairs(argv):
    f=open(argv[0],'r');
    f_o = open(argv[2],'w')
    names=[]
    rows=[]
    cols=[]
    pairs=[]

    #pairwise registration
    for line in f:
        s_line = line.rstrip().rstrip('\n')
        names.append(s_line);
        
        #Get the indices of rows and columns
        iterator = re.finditer(argv[1],s_line)
        for match in iterator:
            #print match.group(1)
            rows.append(match.group(1))
            try:
                #print match.group(2)
                cols.append(match.group(2))
            except: cols.append('1')
        
    # Now do the pairing for from_name in names:
    from_ind = -1
    for from_name in names:
        from_ind = from_ind + 1
        to_ind = from_ind
        for to_name in names[from_ind+1:]:
            to_ind = to_ind + 1
        
            adj_row = False
            same_row = False
            adj_col = False
            same_col = False

            # Check the rows
            try:
                row_from=int(rows[from_ind])
                row_to = int(rows[to_ind])
            except:
                row_from= char_to_num(rows[from_ind])
                row_to = char_to_num(rows[to_ind])
            
            if (row_from == row_to): same_row = True
            elif ( row_from - row_to == 1 or row_from - row_to == -1): adj_row = True
            # Check the cols
            try:
                col_from=int(cols[from_ind])
                col_to = int(cols[to_ind])
            except:
                col_from=char_to_num(cols[from_ind])
                col_to = char_to_num(cols[to_ind])
            if (col_from == col_to): same_col = True
            elif ( col_from - col_to == 1 or col_from - col_to == -1): adj_col = True

            # Now check if the images are adjacent
            if (same_row and adj_col) or (same_col and adj_row):
                pairs.append(from_name+' '+to_name)
                f_o.write(from_name+' '+to_name+'\n');
    return pairs

# The function to perform pairwise registration, joint registration,
# and montaging by taking the first image
def register(pair_list, image_dir):

    #f=open(sys.argv[1],'r');
    f_o = open(sys.argv[2]+'.failed_pairs','w')
    f_xforms = open('xxx_123.txt','w')
    names=[]
    xforms=[]

    #pairwise registration
    for line in pair_list:
        s_line = line.rstrip().rstrip('\n')
        pos = s_line.find(' ')
        from_image = s_line[:pos]
        to_image = s_line[pos+1:]
            
        # perform registration
        success = os.system('register_pair.exe '+image_dir+from_image+' '+ image_dir+to_image +' -remove_2d');
        if success == 0:
            # Add the names to the list if not already there. The list
            # keeps potential images which can be the anchor images for
            # montaging
            found = False 
            for name in names:
                if name == from_image:
                    found = True
                    break
            if not found: names.append(from_image)
        
            found = False 
            for name in names:
                if name == to_image:
                    found = True
                    break
            if not found: names.append(to_image)
        
            # Add the transformed.xml to the list
            from_dot = from_image.find('.')
            to_dot = to_image.find('.');
            f_xforms.write(from_image[:from_dot]+"_to_"+to_image[:to_dot]+"_transform.xml\n")
        else:
            f_o.write("From "+from_image+" to "+to_image+" failed\n")
            #print "From "+from_image+" to "+to_image+" failed"
            
    # Perform joint registration write the xform list to a
    # temporary file and remove it after joint registration
    f_xforms.close()
    os.system('register_joint.exe xxx_123.txt')
    os.system('rm xxx_123.txt')

    # perform montaging using the first image as the anchor
    os.system('mosaic_images.exe joint_transforms.xml '+names[0]+ " -path " + image_dir);


if __name__ == '__main__':
    if len(sys.argv) < 4:
        print 'Usage: '+sys.argv[0]+' image_list filename_pattern out_pair_list'
        sys.exit(1)
    pairs = create_pairs(sys.argv[1:])
    #register(pairs,sys.argv[1])

# Example1: for a 1D image series containing mcnor_5-8-06_arc_bk2_26_ca3_2_7.lsm and mcnor_5-8-06_arc_bk2_26_ca3_2_10.lsm, the filename_pattern is "mcnor_5-8-06_arc_bk2_26_ca3_2_([0-9]*).lsm"

# Example2: for a 2D image series containing Slide61Lbox003F_ChS1-T2.tiff and Slide61Lbox001A_ChS1-T2.tiff, the filename_pattern is "Slide61Lbox00([0-9])([A-Z])_ChS1-T2.tiff"
