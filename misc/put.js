var categoryNb = 0;
var humanNb = 0;
var archiveNb = 0;
var caracNb = {};
var parentNb = {};
caracNb['document'] = 0;
parentNb['document'] = 0;

function addCaracNodes(div, label, nb)
{
    // Add html for carac management
    var carac1 = document.createElement("div");
    carac1.appendChild(document.createTextNode("Caracteristics: "));
    
    var carac2 = document.createElement("input");
    carac2.type = "button";
    carac2.value = "+";
    carac2.name = label + nb + "BtCa"; 
    carac2.onclick = function() { addCarac(label + nb); };
    carac1.appendChild(carac2);
    
    var carac3 = document.createElement("input");
    carac3.type = "button";
    carac3.value = "-";
    carac3.name = label + nb + "BtCd";
    carac3.onclick = function() { delCarac(label + nb); };
    carac1.appendChild(carac3);
    
    var carac4 = document.createElement("div");
    carac4.id = label + nb + "Carac";
    carac1.appendChild(carac4);

    div.appendChild(carac1);
    caracNb[label + nb] = 0;
}

function addParentNodes(div, label, nb)
{
    // Add html for parent's category management
    var parent1 = document.createElement("div");
    parent1.appendChild(document.createTextNode("Into categories: "));
    
    var parent2 = document.createElement("input");
    parent2.type = "button";
    parent2.value = "+";
    parent2.name = label + nb + "BtPa"; 
    parent2.onclick = function() { addParent(label + nb); };
    parent1.appendChild(parent2);
    
    var parent3 = document.createElement("input");
    parent3.type = "button";
    parent3.value = "-";
    parent3.name = label + nb + "BtPd";
    parent3.onclick = function() { delParent(label + nb); };
    parent1.appendChild(parent3);
    
    var parent4 = document.createElement("div");
    parent4.id = label + nb + "Parent";
    parent1.appendChild(parent4);

    div.appendChild(parent1);
    parentNb[label + nb] = 0;
}

function addCategory()
{
    categoryNb=categoryNb+1;
    var div = document.getElementById("category");

    // Append a div node
    var div2 = document.createElement("div")
    div2.id = "category" + categoryNb;
    
    // Create <input> elements, set its type and name attributes

    var hr = document.createElement('b');
    hr.appendChild(document.createTextNode('Category '));
    div2.appendChild(hr);

    var input1 = document.createElement("input");
    input1.type = "text";
    input1.name = "category" + categoryNb + "Label";
    input1.required = "on";
    div2.appendChild(input1);
    
    div2.appendChild(document.createTextNode('top:'));
    
    var input2 = document.createElement("input");
    input2.type = "checkbox";
    input2.name = "category" + categoryNb + "Top";
    input2.value = "on";
    div2.appendChild(input2);

    // Add html for carac management
    addCaracNodes(div2, 'category', categoryNb);

     // Add html for parent's category management
    addParentNodes(div2, 'category', categoryNb);

    div2.appendChild(document.createElement('br'));
    div.appendChild(div2);
}

function delCategory()
{
    var div = document.getElementById("category");
    var div2 = document.getElementById("category" + categoryNb);
    div.removeChild(div2);
    categoryNb=categoryNb-1;
}

function addHuman()
{
    humanNb=humanNb+1;
    var div = document.getElementById("human");

    // Append a node
    var div2 = document.createElement("div")
    div2.id = "human" + humanNb;
    
    // Create <input> elements, set its type and name attributes
    
    var hr = document.createElement('b');
    hr.appendChild(document.createTextNode('Human '));
    div2.appendChild(hr);

    var input1 = document.createElement("input");
    input1.type = "text";
    input1.name = "human" + humanNb + "FirstName";
    input1.required = "on";
    div2.appendChild(input1);

    var input2 = document.createElement("input");
    input2.type = "text";
    input2.name = "human" + humanNb + "SecondName";
    div2.appendChild(input2);

    div2.appendChild(document.createTextNode('as'));

    var input3 = document.createElement("input");
    input3.type = "text";
    input3.name = "human" + humanNb + "Role";
    input3.required = "on";
    div2.appendChild(input3);

    // Add html for carac management
    addCaracNodes(div2, 'human', humanNb);

    // Add html for parent's category management
    addParentNodes(div2, 'human', humanNb);

    div2.appendChild(document.createElement('br'));
    div.appendChild(div2);
}

function delHuman()
{
    var div = document.getElementById("human");
    var div2 = document.getElementById("human" + humanNb);
    div.removeChild(div2);
    humanNb=humanNb-1;
}

function addArchive()
{
    archiveNb=archiveNb+1;
    var div = document.getElementById("archive");
    
    // Append a node
    var div2 = document.createElement("div")
    div2.id = "archive" + archiveNb;
    
    // Create <input> elements, set its type and name attributes
    
    var hr = document.createElement('b');
    hr.appendChild(document.createTextNode('Archive '));
    div2.appendChild(hr);

    var input1 = document.createElement("input");
    input1.type = "file";
    input1.name = "archive" + archiveNb + "Source";
    input1.size = 40;
    input1.required = "on";
    div2.appendChild(input1);

    div2.appendChild(document.createElement('br'));
    var italic = document.createElement('i');
    italic.appendChild(document.createTextNode('target path: '));
    div2.appendChild(italic);
    
    var input2 = document.createElement("input");
    input2.type = "text";
    input2.name = "archive" + archiveNb + "Target";
    div2.appendChild(input2);

    // Add html for carac management
    addCaracNodes(div2, 'archive', archiveNb);

    div2.appendChild(document.createElement('br'));
    div.appendChild(div2);
}

function delArchive()
{
    if (archiveNb == 1) return;

    var div = document.getElementById("archive");
    var div2 = document.getElementById("archive" + archiveNb);
    div.removeChild(div2);
    archiveNb=archiveNb-1;
}

function addCarac(label)
{
    caracNb[label]=caracNb[label]+1;
    var div = document.getElementById(label + "Carac");

    // Append a div label
    var div2 = document.createElement("div")
    div2.id = label + "Carac" + caracNb[label];
    div.appendChild(div2);
    
    // Create an <input> elements, set its type and name attributes
    var input1 = document.createElement("input");
    input1.type = "text";
    input1.name = label + "Carac" + caracNb[label] + "Label";
    input1.required = "on";
    div2.appendChild(input1);

    div2.appendChild(document.createTextNode('is'));

    var input2 = document.createElement("input");
    input2.type = "text";
    input2.name = label + "Carac" + caracNb[label] + "Value";
    input2.required = "on";
    div2.appendChild(input2);
}

function delCarac(label)
{
    var div = document.getElementById(label + "Carac");
    var div2 = document.getElementById(label + "Carac" + caracNb[label]);
    div.removeChild(div2);
    caracNb[label]=caracNb[label]-1;
}

function addParent(label)
{
    parentNb[label]=parentNb[label]+1;
    var div = document.getElementById(label + "Parent");

    // Append a div label
    var div2 = document.createElement("div")
    div2.id = label + "Parent" + parentNb[label];
    div.appendChild(div2);
    
    // Create an <input> elements, set its type and name attributes
    var input1 = document.createElement("input");
    input1.name = label + "Parent" + parentNb[label] + "Label";
    input1.required = "on";
    div2.appendChild(input1);
}

function delParent(label)
{
    var div = document.getElementById(label + "Parent");
    var div2 = document.getElementById(label + "Parent" + parentNb[label]);
    div.removeChild(div2);
    parentNb[label]=parentNb[label]-1;
}
