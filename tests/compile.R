
cexsize <- 1.5 #Handles text scaling
pchvals <- c(3,1,4,5)

device.height <- 7.5
device.width <- 11
pdf.device <- function(file, title, height = device.height,
                       width = device.width) {
  pdf(file=paste(file,".pdf",sep=""), title = title,
      width=device.width, height=height, paper="USr")
}

eps.device <- function(file, title, height = device.height,
                       width = device.width) {
  postscript(file = paste(file,".eps",sep=""),
             width=device.width, height=height, paper="letter",
             onefile = T,
             title = title)
}

printdevice <- eps.device


tables.to.bytes <- function(bits, rules, numtables) {
  N <- 8 * ceiling(rules / 8)
  nt <- numtables
  even <- (nt - (bits %% nt))*(2^(bits %/% nt))*N
  odd <- (bits %% nt)*(2^((bits %/% nt) + 1))*N
  return((even + odd) / 8)
}

#returns the mean a list of data frames passed in
mean.data <- function(xs){
  ms <- merge.all(xs)
  return(aggregate.data.frame(ms
                              ,by = list(
                                 ms$number.of.rules
                                 ,ms$packet.size.in.bits
                                 ,ms$memory.used)
                              ,mean
                              )
         )
}
#pares non-numeric columns from a data.frame
rem.nonnumeric <- function(x){
  data.frame(memory.allowed = x$memory.allowed
             ,memory.used = x$memory.used
             ,number.of.rules = x$number.of.rules
             ,number.of.tables = x$number.of.tables
             ,cpu.process.time = x$cpu.process.time
             ,kbps = x$kbps
             ,packet.size.in.bits = x$packet.size.in.bits
             ,table.build.time = x$table.build.time
             ,total.run.time = x$total.run.time
             ,real.process.time = x$real.process.time
             ,policy.read.time = x$policy.read.time
             ,pps = x$pps
             )
}
#returns a data frame consisting of the merged data from all files patching the
#pattern
loadin <- function(pattern){
  f <- lapply(c(list.files(pattern = pattern)), read.csv, head=T)
  f.n <- lapply(f, rem.nonnumeric)
  return(mean.data(f.n))
}
# special version of loadin that thins out lower end of 12K graphs since they
# were tested unevenly. This should only be used for plotting, the data points
# are valid and should be included in aggregate statistics
loadin.12K <- function(pattern){
  f <- lapply(c(list.files(pattern=pattern)), read.csv, head=T)
  f.n <- lapply(f, rem.nonnumeric)
  swissed <- lapply(f.n,
                    function(x){ #trim some fat
                      x[c(  1: 50,seq( 51,101,15),
                          102:151,seq(152,202,15),
                          203:252,seq(253,303,15),
                          304:353,seq(354,404,15)
                          ),]
                    }
                    )
  return(mean.data(swissed))
}

#takes a data frame and splits it along the number.of.rules factor and returns a
#list of data frames
splitup.by <- function(x, field) {
  xlevels <- unique(x[field])
  y <- list()
  for( i in 1:length(xlevels[[1]])){
    y[[i]] <- subset(x, x[field] == xlevels[[1]][[i]], all=T)
  }
  return(y)
}
# merges all data frames in the passed in list
merge.all <- function(x) {
  merged <- x[[1]]
  if(length(x) > 1){
    for(i in 2:length(x)){
      merged <- merge(merged, x[[i]], all=T)
    }
  }
  return(merged)
}

# adds thousands separators to large numbers, suitable for graphs
comma <- function(x){
  return (formatC(x,format="d", big.mark=","))
}
# adds dots for thousands separators, suitable for filenames
dot <- function(x){
  return (formatC(x,format="d", big.mark="."))
}
# simple mnemonic functions to cut down on the number of zeroes needed to be
# read
thou <- function(x){
  return(x*1000)
}
mil <- function(x){
  return (x*1000000)
}
bil <- function(x){
  return (x*1000000000)
}
# formats with 1 significant digit
digit.1 <- function(x){
  return(formatC(x, format ="f", digit=1, drop0trailing=TRUE))
}
# adds K,M, or B suffix as needed (useful in axis labels)
sci.suffix <- function(xs){
  suffize <- function(x){
    if(x %/% bil(1) != 0){
      return(paste(digit.1(x / bil(1)), "G", sep=""))
    }
    if(x %/% mil(1) != 0){
      return(paste(digit.1(x / mil(1)), "M", sep=""))
    }
    if(x %/% thou(1) != 0){
      return(paste(digit.1(x / thou(1)), "K", sep=""))
    }#if all else fails
    return(digit.1(x))
  }
  return(lapply(xs, suffize))
}
byte.suffix <- function(xs){
  return(lapply(xs, function(x) paste(sci.suffix(x), "B",sep="")))
}
bps.suffix <- function(xs){
  return(lapply(xs, function(x) paste(sci.suffix(x), "bps", sep="")))
}

# adds a field to the data set x that contains real pps
add.real.pps <- function (x) {
  if(x$packet.size.in.bits[1] == 12000){
    x$real.pps <- thou(10) / x$real.process.time
  }else{
    x$real.pps <- thou(500) / x$real.process.time
  }
  return(x)
}

# Really should pull the "list" processing aspect out of this function, and just
# have it do one graph at a time, but currently pressed for time
real.pps.vs.tables <- function(pattern
                               ,bits
                               ,bps.line = 3
                               ,pps.line = 3
                               ,mar = c(3,4,5,4)
                               ,pch = 1
                               ,fudge = 0
                               ,tab.ticks = NULL
                               ,tab.labels = NULL
                               ,mem.ticks = NULL
                               ,mem.labels = NULL
                               ,dir="./pdfs"){
  if(bits == 12000){
    total <- loadin.12K(pattern)
  }else{
    total <- loadin(pattern)
  }
  real.pps.ed <- add.real.pps(total)
  rsp <- splitup.by(real.pps.ed, "number.of.rules")
  pps.label <- "Throughput (PPS)"
  mbps.label <- "Throughput (Mbps)"
  tab.label <- "Number of Tables"
  mem.label <- "Memory Used"
  for(i in 1:length(rsp)){
    numrules <- rsp[[i]]$number.of.rules[1]
    filename <- paste("Tables_vs_PPS_",dot(bits),"bits_",
                      dot(numrules),"rules",sep="")
    titletext <- paste("Throughputs for", comma(bits),
                       "Bits Classified, with", comma(numrules),"Rules")
    printdevice(paste(dir,filename,sep="/"), title = titletext)
    par(mar=mar, cex=cexsize, bty="o")
    options(scipen=10)
    plot(x = rsp[[i]]$number.of.tables, y = rsp[[i]]$real.pps,
         xlab = "", ylab = "", xaxt="n",
         yaxt="n", tck=0, log="", pch = pch)
    if(is.null(tab.ticks)){
      tab.ticks2 <- pretty(rsp[[i]]$number.of.tables, 11)
    }else{
      tab.ticks2 <- tab.ticks[[i]]
    }
    if(is.null(tab.labels)){
      tab.labels2 <- sci.suffix(tab.ticks2)
    }else{
      tab.labels2 <- tab.labels[[i]]
    }
    if(is.null(mem.ticks)){
      mem.ticks2 <- tab.ticks2 + fudge
    }else{
      mem.ticks2 <- mem.ticks[[i]]
    }
    if(is.null(mem.labels)){
      mem.labels2 <- byte.suffix(tables.to.bytes(bits, numrules, mem.ticks2))
    }else{
      mem.labels2 <- mem.labels[[i]]
    }
    pps.ticks <- pretty(rsp[[i]]$real.pps, 10)
    axis(1, tab.ticks2, labels = tab.labels2)
    mtext(1, text = tab.label, line = 2, cex=cexsize)
    axis(2, pps.ticks, labels = sci.suffix(pps.ticks), las = 1)
    mtext(2, text = pps.label, line = pps.line, cex = cexsize)
    axis(4, at = pps.ticks, labels = pps.ticks * 0.012, las=1)
    mtext(4, text = mbps.label, line=bps.line, cex=cexsize)
    axis(3, at = mem.ticks2, labels = mem.labels2, cex.axis = 0.9)
    mtext(3, text = mem.label, line = 2, cex=cexsize)
    dev.off()
  }
}

#specified
real.pps.vs.tables.104 <- function(){
  real.pps.vs.tables(pattern = "alltest104bits_[1-3].csv"
                     ,bits = 104
                     ,pps.line = 3.2
                     ,pch = 3
                     )
}
real.pps.vs.tables.320 <- function(){
  tab.ticks <- c(26,40,55,70,85,100,115,130,145,160)
  tab.labels <- sci.suffix(tab.ticks)
  mem.ticks <- c(26,45,64,83,102,121,140,160)
  mem.labels <- byte.suffix(tables.to.bytes(320,thou(100),mem.ticks))
  real.pps.vs.tables(pattern = "alltest320bits_[1-3].csv"
                     ,bits = 320
                     ,pch = 4
                     ,fudge = 5
                     ,tab.ticks = list(1,1,1
                        ,tab.ticks
                        ,1)
                     ,tab.labels = list(1,1,1
                        ,tab.labels
                        ,1)
                     ,mem.ticks = list(1,1,1
                        ,mem.ticks
                        ,1)
                     ,mem.labels = list(1,1,1
                        ,mem.labels
                        ,1)
                     )
}
real.pps.vs.tables.12K <- function(){
  tab.ticks <- c(1160,1644,2128,2612,3096,3580,4064,4584,5068,5552,6000)
  tab.labels <- sci.suffix(tab.ticks)
  #these have been carefully crafted...
  mem.ticks <- c(1160,1764,2400,2972,3576,4180,4784,5388,6000)
  mem.labels <- byte.suffix(tables.to.bytes(thou(12), thou(10), mem.ticks))
  real.pps.vs.tables(pattern = "maxtest_[1-3].csv"
                     ,bits = 12000
                     ,bps.line = 2.3
                     ,mar = c(3,4,5,3.3)
                     ,pch = 5
                     ,tab.ticks = list(1,1
                        ,tab.ticks
                        ,1)
                     ,tab.labels = list(1,1
                        ,tab.labels
                        ,1)
                     ,mem.ticks = list(1,1
                        ,mem.ticks
                        ,1)
                     ,mem.labels = list(1,1
                        ,mem.labels
                        ,1)
                     )
}
#really... convenient
real.pps.vs.tables.all <- function(){

  #real.pps.vs.tables.32()
  #real.pps.vs.tables.104()
  real.pps.vs.tables.320()
  real.pps.vs.tables.12K()
}

classifier.throughputs <- function(dir = "./pdfs"){
  # plots the maxes and mins of different test runs
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                 )
  #add real.pps column to each total
  pps.ed <- lapply(totals,add.real.pps)
  #split each by number of rules
  rulesplit <- lapply(pps.ed, splitup.by, field="number.of.rules")
  #now get min and max for each rule/bit element
  minmaxed <- list()
  for( i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="memory.used")
  }
  #remerge the results
  rmgd <- lapply(minmaxed, merge.all)
  titletext <- "Classifier Throughputs"
  #draw the plot
  printdevice(paste(dir,"classifier_throughputs",sep="/"), titletext)
  options(scipen = 10)
  xlimit <- c(50, mil(1.5))
  ylimit <- c(100, mil(10))
  par(cex=cexsize, bty="u", mar=c(3,3,1,4.5), tcl=-0.3, ann = F)
  plot(x = rmgd[[1]]$number.of.rules, y = rmgd[[1]]$real.pps,
       xlim = xlimit, ylim = ylimit, 
       log="xy", pch = pchvals[1], xaxt="n", yaxt="n", tck = 0
       )
  pps.ticks <- c(100, thou(1), thou(10), thou(100), mil(1), mil(10))
  bps.ticks <- pps.ticks/1.2 #ticks are in pps scale
  rule.ticks <- unique(rmgd[[1]]$number.of.rules)
  bps.labels <- bps.suffix(bps.ticks*thou(12)) # labels are back in bps scale (a
                                               # packet is 12K bits)
  pps.labels <- sci.suffix(pps.ticks)
  rule.labels <- sci.suffix(rule.ticks)
  axis(1, at = rule.ticks,labels = rule.labels)
  mtext(1, text = "Number of Rules", cex = cexsize, line = 2)
  axis(2, at = pps.ticks, labels = pps.labels)
  mtext(2, text = "Throughput (PPS)", cex = cexsize, line = 2)
  axis(4, at = bps.ticks, labels = bps.labels,las=1)
  for(i in 2:length(rmgd)){
      points(x = rmgd[[i]]$number.of.rules, y = rmgd[[i]]$real.pps,
             pch = pchvals[i])
  }
  legend(x="bottomleft",legend=c(32,104,320,12000),
         pch=pchvals, title="Bits Classified")
  dev.off()
}

#returns a data set with only the minimum and maximum of the given field
minmax <- function(x, field) {
  x.min <- subset(x, x[field] == min(x[field]))[1,]
  x.max <- subset(x, x[field] == max(x[field]))[1,]
  x.final <- merge(x.min, x.max, all=T)
}

#special graph of min/max table build times for all bit sizes vs. number of rules
build.time.vs.rules <- function (dir = "./pdfs") {
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin("maxtest_[1-3].csv")
                )
  #split each data set up by the number of rules
  rulesplit <- lapply(totals, splitup.by, field="number.of.rules")
  #then throw away all but the min and max
  minmaxed <- list()
  for(i in 1:length(rulesplit)){
    minmaxed[[i]] <- lapply(rulesplit[[i]], minmax, field="table.build.time")
  }
  # then re-merge the results
  rmgd <- lapply(minmaxed, merge.all)
  titletext <- "Table-build Times"
  
  printdevice(paste(dir,"build_time_vs_rules",sep="/"), titletext)
  options(scipen = 10)
  par(cex = 1.5 , bty="l",
      mar=c(3,3,1,1), ann = F)
  plot(x = rmgd[[1]]$number.of.rules, y = rmgd[[1]]$table.build.time, log="xy",
       xlim = c(100,1000000), ylim = c(0.001,2000),
       pch = pchvals[1], axes=T, xaxt="n", yaxt="n", tck=0.0)
  yticks <- c(0.001, 0.01, 0.1, 1, 10, 100, 1000)
  xticks <- c(100, thou(1), thou(10), thou(100), mil(1))
  xlabels <- sci.suffix(xticks)
  axis(1, at=xticks, labels = xlabels)
  mtext(1, text = "Number of Rules", cex = cexsize, line = 2)
  axis(2, at=yticks, labels = prettyNum(yticks, drop0trailing = T))
  mtext(2, text ="Table-build Time (s)", cex = cexsize, line = 2 )
  for( i in 2:length(rmgd)){
    points(x = rmgd[[i]]$number.of.rules, y = rmgd[[i]]$table.build.time,
           pch = pchvals[i])
  }
  legend(x="bottomright",legend=c(32,104,320,12000),
         pch=pchvals, title="Bits Classified")
  dev.off()
}

series.of.4.graph <- function(numrules
                              ,xlimits
                              ,ylimits
                              ,xlabels
                              ,bps.label = "Throughput (Gbps)"
                              ,bps.adj = 0.5
                              ,bpsmul = 0.000012
                              ,bps.line = 2
                              ,xlas = 1
                              ,mar = c(3,4,4,4)
                              ,xaxticks = F
                              ,wherelegend = "topright"
                              ,xaxis.line = 2
                              ,pps.line = 2
                              ,pps.adj = 0.5
                              ,dir="./pdfs"
                              ,height = device.height
                              ){
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin.12K("maxtest_[1-3].csv")
                 )
  titletext <- paste("Throughputs for", comma(numrules),"Rules")
  pps.ed <- lapply(totals, add.real.pps)
  only <- lapply(pps.ed, function(x) subset(x, x$number.of.rules == numrules))
  printdevice(paste(dir,paste("4series_",dot(numrules),"_rules",sep=""),sep="/"),
             titletext)
  options(scipen = 10)# only shorten to scientific notation if difference is >
                      # 10 characters
  par(cex = cexsize , bty="u", mar = mar, ann = F)
  plot(x = only[[1]]$memory.used, y = only[[1]]$real.pps, log="x",
       xlim = xlimits, ylim = ylimits,
       pch = pchvals[1],
       xaxt="n",yaxt="n", tck=0.0
       )

  xlab <- "Memory Used"
  ppslabel <- "Throughput (PPS)"
  pps.ticks <- pretty(ylimits, 10)
  if(xaxticks[1] == F){
    axis(1, at = axTicks(1,log=T), labels = byte.suffix(axTicks(1,log=T)), las = xlas)
  }else{
    axis(1, at = xaxticks, labels = byte.suffix(xaxticks), las = xlas)
  }
  mtext(1, text = xlab, line = xaxis.line , cex = cexsize)
  axis(2, at = pps.ticks, labels = sci.suffix(pps.ticks), las=1)
  mtext(2, text = ppslabel, line = pps.line, cex = cexsize, adj = pps.adj)
  axis(4, at = pps.ticks, labels = pps.ticks * bpsmul, las = 1)
  mtext(4, text = bps.label, line = bps.line, cex = cexsize, adj = bps.adj)
  for( i in 2:length(only)){
    points(x = only[[i]]$memory.used, y = only[[i]]$real.pps, pch = pchvals[i])
  }
  legend(x=wherelegend, legend=c(32,104,320,12000),
         pch=pchvals, title="Bits Classified")
  dev.off()
}

exp.4.graph <- function(numrules
                        ,xlimits
                        ,ylimits
                        ,xlabels
                        ,bps.label = "Throughput (Gbps)"
                        ,bps.adj = 0.5
                        ,bpsmul = 0.000012
                        ,bps.line = 2
                        ,xlas = 1
                        ,mar = c(3,4,4,4)
                        ,xaxticks = F
                        ,wherelegend = "topright"
                        ,xaxis.line = 2
                        ,pps.line = 2
                        ,pps.adj = 0.5
                        ,dir="./pdfs"
                        ,relheights = c(0.92,0.92)
                        ){
  totals <- list(  loadin("alltest32bits_[1-3].csv")
                 , loadin("alltest104bits_[1-3].csv")
                 , loadin("alltest320bits_[1-3].csv")
                 , loadin.12K("maxtest_[1-3].csv")
                 )
  titletext <- paste("Throughputs for", comma(numrules),"Rules")
  pps.ed <- lapply(totals, add.real.pps)
  only <- lapply(pps.ed, function(x) subset(x, x$number.of.rules == numrules))
  only.1 <- lapply(only, function(x) subset(x, x$real.pps <= ylimits[2]))
  only.2 <- lapply(only, function(x) subset(x, x$real.pps >= ylimits[3]))
  printdevice(paste(dir,paste("4series_",dot(numrules),"_rules",sep=""),sep="/"),
             titletext)
  options(scipen = 10)# only shorten to scientific notation if difference is >
                      # 10 characters
  par(cex = cexsize , bty="u", ann = F, mar=c(0,0,0,0))#, mar = mar
  mrx <- c(mar[2], -mar[4], mar[1], -mar[3]) * 0.03
  
  pps.ticks1 <- pretty(ylimits[3:4], 1)
  pps.ticks2 <- pretty(ylimits[1:2], 10)
  #plot first group
  par(fig=c(0, 1, relheights[2], 1) + c(mrx[1],mrx[2],0,mrx[4]), bty="n")
  plot(x = only.2[[1]]$memory.used, y = only.2[[1]]$real.pps, log="x",
       xlim = xlimits, ylim = ylimits[3:4], pch=pchvals[1],
       xaxt="n",yaxt="n",tck=0.0)
  for(i in 2:length(only.2)){
    points(x = only.2[[i]]$memory.used, y = only.2[[i]]$real.pps, pch = pchvals[i])
  }
  axis(2, at = pps.ticks1, labels = sci.suffix(pps.ticks1), las=1)
  axis(4, at = pps.ticks1, labels = pps.ticks1 * bpsmul, las = 1)
  lines(x=c(xlimits[1]*0.5,xlimits[2]*1.6), y=rep(ylimits[3],2), lty="dashed")
  #plot second group
  par(fig=c(0, 1, 0, relheights[1]) + c(mrx[1],mrx[2],mrx[3],0), new=T,
      lty="solid", bty="u")
  plot(x = only.1[[1]]$memory.used, y = only.1[[1]]$real.pps, log="x",
       xlim = xlimits, ylim = ylimits[1:2],
       pch = pchvals[1],
       xaxt="n",yaxt="n", tck=0.0
       )

  xlab <- "Memory Used"
  ppslabel <- "Throughput (PPS)"
  if(xaxticks[1] == F){
    axis(1, at = axTicks(1,log=T), labels = byte.suffix(axTicks(1,log=T)), las = xlas)
  }else{
    axis(1, at = xaxticks, labels = byte.suffix(xaxticks), las = xlas)
  }
  mtext(1, text = xlab, line = xaxis.line , cex = cexsize)
  axis(2, at = pps.ticks2, labels = sci.suffix(pps.ticks2), las=1)
  mtext(2, text = ppslabel, line = pps.line, cex = cexsize, adj = pps.adj)
  axis(4, at = pps.ticks2, labels = pps.ticks2 * bpsmul, las = 1)
  mtext(4, text = bps.label, line = bps.line, cex = cexsize, adj = bps.adj)
  for( i in 2:length(only.1)){
    points(x = only.1[[i]]$memory.used, y = only.1[[i]]$real.pps, pch = pchvals[i])
  }
  legend(x=wherelegend, inset=0.02, legend=c(32,104,320,12000),
         pch=pchvals, title="Bits Classified")

  dev.off()
}

exp.4.graph.1K <- function() {
  exp.4.graph(numrules = thou(1)
              ,xlimits = c(thou(10), bil(2))
              ,ylimits = c(0, mil(1.78), mil(3.5), mil(4.5))
              ,mar = c(4,4.1,0,3.8)
              ,relheights = c(0.92,0.92)
              ,pps.line = 3.1
              ,bps.line = 2.3
              ,bps.adj = 0.45
              )
}

exp.4.graph.10K <- function(){
  exp.4.graph(numrules = thou(10)
              ,xlimits = c(thou(50), bil(2))
              ,ylimits = c(1, thou(235), mil(2.1),mil(2.6))
              ,wherelegend = "bottomleft"
              ,mar = c(3.9,4,0,3.8)
              ,pps.line = 3
              ,bps.line = 2.8
              )
}

exp.4.graph.100K <- function(){
  exp.4.graph(numrules = thou(100)
              ,xlimits = c(mil(1), bil(2))
              ,xaxticks = c(mil(1),mil(3),mil(10),mil(30),mil(100),mil(300),bil(1))
              ,ylimits = c(1, thou(30), thou(805), thou(860))
              ,bpsmul = 0.012
              ,bps.label = "Throughput (Mbps)"
              ,wherelegend = "bottomleft"
              ,xlas = 1
              ,xaxis.line = 2
              ,pps.line = 2.8
              ,bps.line = 2.6
              ,mar = c(4,3.8,0,3.6)
              )
}
# convenience function for drawing all specialized graphs
series.of.4.graph.all <- function(){
  exp.4.graph.100K()
  exp.4.graph.10K()
  exp.4.graph.1K()
}

#super convenient and slow
all.graphs <- function() {
  series.of.4.graph.all()
  real.pps.vs.tables.all()
  build.time.vs.rules()
  classifier.throughputs()
}

# axis.break places a break marker at the position "breakpos" 
# in user coordinates on the axis nominated - see axis().

axis.break<-function(axis=1,breakpos,bgcol="white",breakcol="black",
 style=c("slash","zigzag"),brw=0.02) {
 
 if(!missing(breakpos)) {
  # get the coordinates of the outside of the plot
  figxy<-par("usr")
  # flag if either axis is logarithmic
  xaxl<-par("xlog")
  yaxl<-par("ylog")
  # calculate the x and y offsets for the break
  xw<-(figxy[2]-figxy[1])*brw
  yw<-(figxy[4]-figxy[3])*brw
  if(xaxl && (axis == 1 || axis == 3)) breakpos<-log10(breakpos)
  if(yaxl && (axis == 2 || axis == 4)) breakpos<-log10(breakpos)
  # set up the "blank" rectangle (left, bottom, right, top)
  switch(axis,
   br<-c(breakpos-xw/2,figxy[3]-yw/2,breakpos+xw/2,figxy[3]+yw/2),
   br<-c(figxy[1]-xw/2,breakpos-yw/2,figxy[1]+xw/2,breakpos+yw/2),
   br<-c(breakpos-xw/2,figxy[4]-yw/2,breakpos+xw/2,figxy[4]+yw/2),
   br<-c(figxy[2]-xw/2,breakpos-yw/2,figxy[2]+xw/2,breakpos+yw/2),
   stop("Improper axis specification."))
  # get the current setting of xpd
  old.xpd<-par("xpd")
  # don't cut the break off at the edge of the plot
  par(xpd=T)
  # correct for logarithmic axes
  if(xaxl) br[c(1,3)]<-10^br[c(1,3)]
  if(yaxl) br[c(2,4)]<-10^br[c(2,4)]
  # draw the "blank" rectangle
  rect(br[1],br[2],br[3],br[4],col=bgcol,border=bgcol)
  if(style == "slash") {
   # calculate the slash ends
   if(axis == 1 || axis == 3) {
    xbegin<-c(breakpos-xw,breakpos)
    xend<-c(breakpos,breakpos+xw)
    ybegin<-c(br[2],br[2])
    yend<-c(br[4],br[4])
    if(xaxl) {
     xbegin<-10^xbegin
     xend<-10^xend
    }
   }
   else {
    xbegin<-c(br[1],br[1])
    xend<-c(br[3],br[3])
    ybegin<-c(breakpos-yw,breakpos)
    yend<-c(breakpos,breakpos+yw)
    if(yaxl) {
     ybegin<-10^ybegin
     yend<-10^yend
    }
   }
  }
  else {
   # calculate the zigzag ends
   if(axis == 1 || axis == 3) {
    xbegin<-c(breakpos-xw/2,breakpos-xw/4,breakpos+xw/4)
    xend<-c(breakpos-xw/4,breakpos+xw/4,breakpos+xw/2)
    ybegin<-c(ifelse(yaxl,10^figxy[3+(axis==3)],figxy[3+(axis==3)]),br[4],br[2])
    yend<-c(br[4],br[2],ifelse(yaxl,10^figxy[3+(axis==3)],figxy[3+(axis==3)]))
    if(xaxl) {
     xbegin<-10^xbegin
     xend<-10^xend
    }
   }
   else {
    xbegin<-c(ifelse(xaxl,10^figxy[1+(axis==4)],figxy[1+(axis==4)]),br[1],br[3])
    xend<-c(br[1],br[3],ifelse(xaxl,10^figxy[1+(axis==4)],figxy[1+(axis==4)]))
    ybegin<-c(breakpos-yw/2,breakpos-yw/4,breakpos+yw/4)
    yend<-c(breakpos-yw/4,breakpos+yw/4,breakpos+yw/2)
    if(yaxl) {
     ybegin<-10^ybegin
     yend<-10^yend
    }
   }
  }
  # draw the segments
  segments(xbegin,ybegin,xend,yend,col=breakcol,lty=1)
  # restore xpd
  par(xpd=old.xpd)
 }
 else {
  cat("Usage: axis.break(axis=1,breakpos,bgcol=\"white\",breakcol=\"black\",\n")
  cat("\tstyle=c(\"slash\",\"zigzag\"),brw=0.02)\n")
 }
}
