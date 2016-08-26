# Change Files Location drush script
Drupal module stub around a drush script that allows changing files locations for fields, and moves all related files to that new location.

## Usage
```
# drush help fdu

Examples:
 drush field-file-directory-update node  Change the file directory for the  
 affiliate field_affiliate_image         field_affiliate_image field of the 
 --path='affiliate'                      affiliate content type.            


Arguments:
 type                                      The type of the entity the field is 
                                           attached to.                        
 bundle                                    The bundle this field belongs to.   
 field                                     The name of an existing field.      


Options:
 --path                                    Optional subdirectory within the    
                                           upload destination where files will 
                                           be stored. Do not include preceding 
                                           or trailing slashes. This field     
                                           supports tokens.                    


Aliases: fdu
```